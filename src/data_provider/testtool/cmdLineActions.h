/*
 * Wazuh SYSINFO
 * Copyright (C) 2015-2021, Wazuh Inc.
 * September 13, 2021
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _CMD_LINE_ACTIONS_H_
#define _CMD_LINE_ACTIONS_H_

#include <string.h>

constexpr auto HARDWARE_ACTION  { "--hardware"};
constexpr auto NETWORKS_ACTION  { "--networks"};
constexpr auto PACKAGES_ACTION  { "--packages"};
constexpr auto PROCESSES_ACTION { "--processes"};
constexpr auto PORTS_ACTION     { "--ports"};
constexpr auto OS_ACTION        { "--os"};

class CmdLineActions final
{
    public:
        CmdLineActions(const char* argv[])
            : m_hardware  { HARDWARE_ACTION  == std::string(argv[1]) }
            , m_networks  { NETWORKS_ACTION  == std::string(argv[1]) }
            , m_packages  { PACKAGES_ACTION  == std::string(argv[1]) }
            , m_processes { PROCESSES_ACTION == std::string(argv[1]) }
            , m_ports     { PORTS_ACTION     == std::string(argv[1]) }
            , m_os        { OS_ACTION        == std::string(argv[1]) }
        {}

        bool hardwareArg() const
        {
            return m_hardware;
        };

        bool networksArg() const
        {
            return m_networks;
        };

        bool packagesArg() const
        {
            return m_packages;
        };

        bool processesArg() const
        {
            return m_processes;
        };

        bool portsArg() const
        {
            return m_ports;
        };

        bool osArg() const
        {
            return m_os;
        };

        static void showHelp()
        {
            std::cout << "\nUsage: sysinfo_test_tool [options]\n"
                      << "Options:\n"
                      << "\t<without args> \tPrints the complete Operating System information.\n"
                      << "\t--hardware \tPrints the current Operating System hardware information.\n"
                      << "\t--networks \tPrints the current Operating System networks information.\n"
                      << "\t--packages \tPrints the current Operating System packages information.\n"
                      << "\t--processes \tPrints the current Operating System processes information.\n"
                      << "\t--ports \tPrints the current Operating System ports information.\n"
                      << "\t--os \t\tPrints the current Operating System information.\n"
                      << "\nExamples:"
                      << "\n\t./sysinfo_test_tool"
                      << "\n\t./sysinfo_test_tool --hardware"
                      << "\n\t./sysinfo_test_tool --networks"
                      << "\n\t./sysinfo_test_tool --packages"
                      << "\n\t./sysinfo_test_tool --processes"
                      << "\n\t./sysinfo_test_tool --ports"
                      << "\n\t./sysinfo_test_tool --os"
                      << std::endl;
        }

    private:
        const bool m_hardware;
        const bool m_networks;
        const bool m_packages;
        const bool m_processes;
        const bool m_ports;
        const bool m_os;
};

#endif // _CMD_LINE_ACTIONS_H_
