/*
 * tool_paths.h — Cross-platform tool discovery for OpenOCD and STM32CubeProgrammer
 */

#ifndef TOOL_PATHS_H
#define TOOL_PATHS_H

#include <QString>

namespace ToolPaths {

/*
 * Locate the OpenOCD binary.  Search order:
 *   1. App-local bundle (<app>/openocd/bin/openocd[.exe])
 *   2. STM32CubeIDE installation (platform-specific paths)
 *   3. System PATH
 * Returns empty string if not found.
 */
QString findOpenOcd();

/*
 * Given a resolved OpenOCD binary path, locate its scripts directory.
 * Checks for sibling scripts/ dir (bundled), IDE plugin layouts, and
 * ../share/openocd/scripts/ (system PATH installs).
 * Returns empty string if not found.
 */
QString findOpenOcdScripts(const QString &ocdBinaryPath);

/*
 * Locate STM32_Programmer_CLI.  Search order:
 *   1. STM32CubeProgrammer standalone install (platform-specific paths)
 *   2. STM32CubeIDE embedded cubeprogrammer plugin
 *   3. System PATH
 * Returns empty string if not found.
 */
QString findCubeProgrammerCli();

} // namespace ToolPaths

#endif // TOOL_PATHS_H
