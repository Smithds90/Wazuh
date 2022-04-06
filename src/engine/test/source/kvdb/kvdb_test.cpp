#include <filesystem>
#include <thread>
#include <condition_variable>
#include <random>

#include <gtest/gtest.h>
#include <pthread.h> //For barrier, not strictly necessary

#include <kvdb/kvdbManager.hpp>
#include <logging/logging.hpp>

// TODO: can we move this utility functions to headers accessible to tests and
// benchmark?
static std::string getRandomString(int len, bool includeSymbols = false)
{
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    static const char symbols[] = "-_'\\/. *!\"#$%&()+[]{},;";

    std::string tmp_s;
    tmp_s.reserve(len);

    std::string dict = alphanum;
    if (includeSymbols)
    {
        dict += symbols;
    }

    for (int i = 0; i < len; ++i)
    {
        tmp_s += dict[rand() % dict.size()];
    }
    return tmp_s;
}
namespace
{
class KVDBTest : public ::testing::Test
{

protected:
    bool initialized = KVDBManager::init("/tmp/");
    KVDBManager &kvdbManager = KVDBManager::get();

    KVDBTest()
    {
        logging::LoggingConfig logConfig;
        logConfig.logLevel = logging::LogLevel::Off;
        logging::loggingInit(logConfig);
    }

    virtual ~KVDBTest()
    { // = default;
    }

    virtual void SetUp()
    {
        kvdbManager.createDB("TEST_DB");
    }

    virtual void TearDown()
    {
        kvdbManager.deleteDB("TEST_DB");
    }
};

TEST_F(KVDBTest, CreateDeleteKvdbFile)
{
    bool ret = kvdbManager.createDB("NEW_DB");
    ASSERT_TRUE(ret);
    auto newKvdb = kvdbManager.getDB("NEW_DB");
    ASSERT_EQ(newKvdb->getName(), "NEW_DB");
    ASSERT_EQ(newKvdb->getState(), KVDB::State::Open);

    kvdbManager.deleteDB("NEW_DB");
    auto deletedKvdb = kvdbManager.getDB("NEW_DB");
    ASSERT_EQ(deletedKvdb, nullptr);
}

TEST_F(KVDBTest, CreateDeleteColumns)
{
    const std::string COLUMN_NAME = "NEW_COLUMN";
    auto kvdb = kvdbManager.getDB("TEST_DB");
    bool ret = kvdb->createColumn(COLUMN_NAME);
    ASSERT_TRUE(ret);
    ret = kvdb->deleteColumn(COLUMN_NAME);
    ASSERT_TRUE(ret);
    ret = kvdb->deleteColumn(COLUMN_NAME);
    ASSERT_FALSE(ret);
}

TEST_F(KVDBTest, ReadWrite)
{
    const std::string KEY = "dummy_key";
    const std::string VALUE = "dummy_value";
    std::string valueRead;
    bool ret;

    auto kvdb = kvdbManager.getDB("TEST_DB");

    ret = kvdb->write(KEY, VALUE);
    ASSERT_TRUE(ret);

    ret = kvdb->hasKey(KEY);
    ASSERT_TRUE(ret);

    valueRead = kvdb->read(KEY);
    ASSERT_EQ(valueRead, VALUE);

    ret = kvdb->readPinned(KEY, valueRead); // Check this...
    ASSERT_TRUE(ret);
    ASSERT_EQ(valueRead, VALUE);

    ret = kvdb->deleteKey(KEY);
    ASSERT_TRUE(ret);

    ret = kvdb->hasKey(KEY);
    ASSERT_FALSE(ret);

    valueRead = kvdb->read(KEY);
    ASSERT_TRUE(valueRead.empty());

    ret = kvdb->readPinned(KEY, valueRead);
    ASSERT_FALSE(ret);
    ASSERT_TRUE(valueRead.empty());
}

// Key-only write
TEST_F(KVDBTest, KeyOnlyWrite)
{
    // TODO Update FH tests too
    const std::string KEY = "dummy_key";
    bool ret;
    auto kvdb = kvdbManager.getDB("TEST_DB");

    ret = kvdb->hasKey(KEY);
    ASSERT_FALSE(ret);
    ret = kvdb->writeKeyOnly(KEY);
    ASSERT_TRUE(ret);
    ret = kvdb->hasKey(KEY);
    ASSERT_TRUE(ret);
}

TEST_F(KVDBTest, ReadWriteColumn)
{
    const std::string COLUMN_NAME = "NEW_COLUMN";
    const std::string KEY = "dummy_key";
    const std::string VALUE = "dummy_value";
    std::string valueRead;
    bool ret;

    auto kvdb = kvdbManager.getDB("TEST_DB");

    ret = kvdb->createColumn(COLUMN_NAME);
    ASSERT_TRUE(ret);

    ret = kvdb->write(KEY, VALUE, COLUMN_NAME);
    ASSERT_TRUE(ret);

    valueRead = kvdb->read(KEY, COLUMN_NAME);
    ASSERT_EQ(valueRead, VALUE);
}

TEST_F(KVDBTest, Transaction_ok)
{
    std::vector<std::pair<std::string, std::string>> vInput = {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"},
        {"key4", "value4"},
        {"key5", "value5"}};
    bool ret;

    auto kvdb = kvdbManager.getDB("TEST_DB");
    ret = kvdb->writeToTransaction(vInput);
    ASSERT_TRUE(ret);
    for (auto pair : vInput)
    {
        std::string valueRead = kvdb->read(pair.first);
        ASSERT_EQ(valueRead, pair.second);
    }
}

TEST_F(KVDBTest, Transaction_invalid_input)
{
    bool ret;
    auto kvdb = kvdbManager.getDB("TEST_DB");

    // Empty input
    std::vector<std::pair<std::string, std::string>> vEmptyInput = {};
    ret = kvdb->writeToTransaction(vEmptyInput);
    ASSERT_FALSE(ret);

    // Invalid Column name
    std::vector<std::pair<std::string, std::string>> vInput = {{"key1", "value1"}};
    ret = kvdb->writeToTransaction(vInput, "InexistentColumn");
    ASSERT_FALSE(ret);

    // Partial input
    std::vector<std::pair<std::string, std::string>> vPartialInput = {
        {"", "value1"},
        {"key2", "value2"}};
    ret = kvdb->writeToTransaction(vPartialInput);
    ASSERT_TRUE(ret);
    std::string valueRead = kvdb->read(vPartialInput[1].first);
    ASSERT_EQ(valueRead, vPartialInput[1].second);
}

// TODO Mock DB and create tests for:
//  Txn start error
//  Put error
//  Commit error

TEST_F(KVDBTest, CleanColumn)
{
    const std::string COLUMN_NAME = "NEW_COLUMN";
    const std::string KEY = "dummy_key";
    const std::string VALUE = "dummy_value";
    std::string valueRead;
    bool ret;

    auto kvdb = kvdbManager.getDB("TEST_DB");

    // default column
    ret = kvdb->write(KEY, VALUE);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY);
    ASSERT_EQ(valueRead, VALUE);
    ret = kvdb->cleanColumn();
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY);
    ASSERT_TRUE(valueRead.empty());

    // custom column
    ret = kvdb->createColumn(COLUMN_NAME);
    ASSERT_TRUE(ret);
    ret = kvdb->write(KEY, VALUE, COLUMN_NAME);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY, COLUMN_NAME);
    ASSERT_EQ(valueRead, VALUE);
    ret = kvdb->cleanColumn(COLUMN_NAME);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY, COLUMN_NAME);
    ASSERT_TRUE(valueRead.empty());
}

TEST_F(KVDBTest, ValueKeyLength)
{
    const std::string KEY = "dummy_key";
    auto kvdb = kvdbManager.getDB("TEST_DB");
    std::string valueRead;
    std::string valueWrite;
    bool ret;

    valueWrite = getRandomString(128, true);
    ret = kvdb->write(KEY, valueWrite);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY);
    ASSERT_EQ(valueWrite, valueRead);

    valueWrite = getRandomString(512, true);
    ret = kvdb->write(KEY, valueWrite);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY);
    ASSERT_EQ(valueWrite, valueRead);

    valueWrite = getRandomString(1024, true);
    ret = kvdb->write(KEY, valueWrite);
    ASSERT_TRUE(ret);
    valueRead = kvdb->read(KEY);
    ASSERT_EQ(valueWrite, valueRead);
}

TEST_F(KVDBTest, ManagerConcurrency)
{
    constexpr static const char *dbName = "test_db";
    constexpr int kMaxTestIterations = 100;

    pthread_barrier_t barrier;
    ASSERT_EQ(0, pthread_barrier_init(&barrier, NULL, 3));

    std::thread create {[&]
                        {
                            auto ret = pthread_barrier_wait(&barrier);
                            EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                        0 == ret);
                            auto &m = KVDBManager::get();
                            for (int i = 0; i < kMaxTestIterations; ++i)
                            {
                                auto db = m.getDB(dbName);
                                if (db && !db->isValid())
                                {
                                    m.createDB(dbName);
                                }
                            }
                        }};

    std::thread read {[&]
                      {
                          auto ret = pthread_barrier_wait(&barrier);
                          EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                      0 == ret);
                          auto &m = KVDBManager::get();
                          for (int i = 0; i < kMaxTestIterations; ++i)
                          {
                              auto db = m.getDB(dbName);
                          }
                      }};

    std::thread del {[&]
                     {
                         auto ret = pthread_barrier_wait(&barrier);
                         EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                     0 == ret);
                         auto &m = KVDBManager::get();
                         for (int i = 0; i < kMaxTestIterations; ++i)
                         {
                             auto db = m.getDB(dbName);
                             if (db && db->isValid())
                             {
                                 m.deleteDB(dbName);
                             }
                         }
                     }};

    create.join();
    read.join();
    del.join();
}

TEST_F(KVDBTest, KVDBConcurrency)
{
    constexpr static const char *dbName = "test_db";
    constexpr int kMaxTestIterations = 100;

    pthread_barrier_t barrier;
    ASSERT_EQ(0, pthread_barrier_init(&barrier, NULL, 4));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution distrib(0, 100);

    KVDBManager::get().createDB(dbName);

    std::thread create {[&]
                        {
                            auto ret = pthread_barrier_wait(&barrier);
                            EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                        0 == ret);
                            auto db = KVDBManager::get().getDB(dbName);
                            for (int i = 0; i < kMaxTestIterations; ++i)
                            {
                                db->createColumn(
                                    fmt::format("colname.{}", distrib(gen)));
                            }
                        }};

    std::thread write {[&]
                        {
                            auto ret = pthread_barrier_wait(&barrier);
                            EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                        0 == ret);
                            auto db = KVDBManager::get().getDB(dbName);
                            for (int i = 0; i < kMaxTestIterations; ++i)
                            {
                                db->write(
                                    fmt::format("key{}", distrib(gen)),
                                    "value",
                                    fmt::format("colname.{}", distrib(gen)));
                            }
                        }};

    std::thread read {[&]
                      {
                          auto ret = pthread_barrier_wait(&barrier);
                          EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                      0 == ret);
                          auto db = KVDBManager::get().getDB(dbName);
                          for (int i = 0; i < kMaxTestIterations; ++i)
                          {
                              db->read(fmt::format("key{}", distrib(gen)),
                                       fmt::format("colname.{}", distrib(gen)));
                          }
                      }};

    std::thread del {[&]
                     {
                         auto ret = pthread_barrier_wait(&barrier);
                         EXPECT_TRUE(PTHREAD_BARRIER_SERIAL_THREAD == ret ||
                                     0 == ret);
                         auto db = KVDBManager::get().getDB(dbName);
                         for (int i = 0; i < kMaxTestIterations; ++i)
                         {
                             db->deleteColumn(
                                 fmt::format("colname.{}", distrib(gen)));
                         }
                     }};

    create.join();
    write.join();
    read.join();
    del.join();
    KVDBManager::get().deleteDB(dbName);
}
}
