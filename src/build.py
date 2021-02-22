import argparse
from ci import utils

class CommandLineParser:

    def _argIsValid(self, arg):
        """
        Checks if the argument being selected is a correct one.

        :param arg: Argument being selected in the command line.
        :return True is 'arg' is a correct one, False otherwise.
        """
        ret = False
        if arg == 'wazuh_modules/syscollector' or arg == 'shared_modules/dbsync' or arg == 'shared_modules/rsync' or arg == 'shared_modules/utils' or arg == 'data_provider':
            # Available modules so far
            ret = True
        return ret

    def processArgs(self):
        """
        Process the command line arguments and executes the corresponding argument's utility.
        """
        action = False
        parser = argparse.ArgumentParser()
        parser.add_argument("-r", "--readytoreview", help="Run all the quality checks needed to create a PR. Example: python3 build.py -r <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("-m", "--make", help="Compile the lib. Example: python3 build.py -m <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("-t", "--tests", help="Run tests (should be configured with TEST=on). Example: python3 build.py -t <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("-c", "--coverage", help="Collect tests coverage and generates report. Example: python3 build.py -c <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("-v", "--valgrind", help="Run valgrind on tests. Example: python3 build.py -v <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("--clean", help="Clean the lib. Example: python3 build.py --clean <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        parser.add_argument("--cppcheck", help="Run cppcheck on the code. Example: python3 build.py --cppcheck <shared_modules/dbsync|shared_modules/rsync|shared_modules/utils|wazuh_modules/syscollector>")
        args = parser.parse_args()

        if self._argIsValid(args.readytoreview):
            utils.runReadyToReview(args.readytoreview)
            action = True
        else:
            if self._argIsValid(args.clean):
                utils.cleanLib(args.clean)
                action = True
            if self._argIsValid(args.make):
                utils.makeLib(args.make)
                action = True
            if self._argIsValid(args.tests):
                utils.runTests(args.tests)
                action = True
            if self._argIsValid(args.coverage):
                utils.runCoverage(args.coverage)
                action = True
            if self._argIsValid(args.valgrind):
                utils.runValgrind(args.valgrind)
                action = True
            if self._argIsValid(args.cppcheck):
                utils.runCppCheck(args.cppcheck)
                action = True
            if not action:
                parser.print_help()

if __name__ == "__main__":
    cmdLineParser = CommandLineParser()
    cmdLineParser.processArgs()
