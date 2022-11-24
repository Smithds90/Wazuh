#include "connection.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <uvw/pipe.hpp>

#include <utils/stringUtils.hpp>

#include "base/utils/getExceptionStack.hpp"

namespace
{
void client(uvw::Loop& loop,
            const std::string& socketPath,
            const std::string& request,
            std::string& response)
{
    auto client = loop.resource<uvw::PipeHandle>();

    client->on<uvw::ErrorEvent>(
        [](const uvw::ErrorEvent& event, uvw::PipeHandle& handle)
        {
            std::cerr << "Engine API Client: An ErrorEvent was raised: " << event.what()
                      << std::endl;
            handle.close();
        });

    client->once<uvw::ConnectEvent>(
        [&request](const uvw::ConnectEvent&, uvw::PipeHandle& handle)
        {
            //std::cout << "API Client ConnectEvent" << std::endl;
            std::vector<char> buffer {request.begin(), request.end()};
            //std::cout << "API Client sending request: " << request << std::endl;
            handle.write(buffer.data(), buffer.size());
            handle.read();
        });

    client->on<uvw::DataEvent>(
        [&response](const uvw::DataEvent& event, uvw::PipeHandle& handle)
        {
            //std::cout << "API Client DataEvent" << std::endl;
            // std::cout << "API Client received response: "
            //           << std::string(event.data.get(), event.length) << std::endl;
            response =
                std::string(event.data.get() + sizeof(int), event.length - sizeof(int));
            handle.close();
        });

    // client->on<uvw::WriteEvent>([](const uvw::WriteEvent&, uvw::PipeHandle&)
    //                             { std::cout << "API Client WriteEvent" << std::endl; });

    client->once<uvw::EndEvent>(
        [](const uvw::EndEvent&, uvw::PipeHandle& handle)
        {
            // std::cout << "API Client EndEvent" << std::endl;
            int count = 0;
            handle.loop().walk([&count](uvw::BaseHandle&) { ++count; });
            // std::cout << "still alive: " << count << " handles" << std::endl;
            handle.close();
        });

    client->once<uvw::CloseEvent>(
        [](const uvw::CloseEvent&, uvw::PipeHandle& handle)
        {
            // std::cout << "API Client CloseEvent" << std::endl;
            int count = 0;
            handle.loop().walk([&count](uvw::BaseHandle&) { ++count; });
            // std::cout << "still alive: " << count << " handles" << std::endl;
        });

    // Send request
    client->connect(socketPath);
}

void dummyServer(uvw::Loop& loop, const std::string& socketPath)
{
    auto server = loop.resource<uvw::PipeHandle>();

    server->on<uvw::ErrorEvent>(
        [](const uvw::ErrorEvent& error, uvw::PipeHandle& handle)
        { std::cerr << "API Server ErrorEvent: " << error.what() << std::endl; });

    server->on<uvw::ListenEvent>(
        [](const uvw::ListenEvent&, uvw::PipeHandle& handle)
        {
            std::cout << "API Server ListenEvent" << std::endl;
            auto client = handle.loop().resource<uvw::PipeHandle>();

            client->on<uvw::ErrorEvent>(
                [](const uvw::ErrorEvent& error, uvw::PipeHandle& handle) {
                    std::cerr << "API Server connection ErrorEvent: " << error.what()
                              << std::endl;
                });

            client->on<uvw::DataEvent>(
                [](const uvw::DataEvent& event, uvw::PipeHandle& handle)
                {
                    std::cout << "API Server connection DataEvent" << std::endl;
                    std::cout.write(event.data.get(), event.length) << std::endl;
                    handle.write(event.data.get(), event.length);
                });

            client->on<uvw::WriteEvent>(
                [](const uvw::WriteEvent& event, uvw::PipeHandle& handle)
                { std::cout << "API Server connection WriteEvent" << std::endl; });

            client->once<uvw::EndEvent>(
                [](const uvw::EndEvent&, uvw::PipeHandle& handle)
                {
                    std::cout << "API Server connection EndEvent" << std::endl;
                    int count = 0;
                    handle.loop().walk([&count](uvw::BaseHandle&) { ++count; });
                    std::cout << "still alive: " << count << " handles" << std::endl;
                    handle.close();
                });

            client->once<uvw::CloseEvent>(
                [ptr = handle.shared_from_this()](const uvw::CloseEvent&,
                                                  uvw::PipeHandle& handle)
                {
                    std::cout << "API Server connection CloseEvent" << std::endl;
                    int count = 0;
                    handle.loop().walk([&count](uvw::BaseHandle&) { ++count; });
                    std::cout << "still alive: " << count << " handles" << std::endl;
                    // ptr->close();
                });

            handle.accept(*client);
            client->read();
        });

    server->once<uvw::CloseEvent>([](const uvw::CloseEvent&, uvw::PipeHandle& handle)
                                  { std::cout << "API Server CloseEvent" << std::endl; });

    server->once<uvw::EndEvent>([](const uvw::EndEvent&, uvw::PipeHandle& handle)
                                { std::cout << "API Server EndEvent" << std::endl; });

    server->bind(socketPath);
    server->listen();
}

} // namespace

namespace cmd::apiclnt
{

std::string connection(const std::string& socketPath, const std::string& request)
{
    // Add protocol header
    int32_t length = request.size();
    std::unique_ptr<char[]> buffer(new char[sizeof(length) + length]);
    std::memcpy(buffer.get(), &length, sizeof(length));
    std::memcpy(buffer.get() + sizeof(length), request.data(), length);
    auto requestWithHeader = std::string(buffer.get(), sizeof(length) + length);

    // Stablish connection, send request and receive response
    std::string response;
    auto loop = uvw::Loop::getDefault();
    client(*loop, socketPath, requestWithHeader, response);
    loop->run();
    return response;
}

} // namespace cmd::apiclnt
