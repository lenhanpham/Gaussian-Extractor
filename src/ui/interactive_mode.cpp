/**
 * @file interactive_mode.cpp
 * @brief Implementation of interactive mode functionality
 * @author Le Nhan Pham
 * @date 2025
 *
 * This file contains the implementation of functions for the interactive
 * command-line interface of the Gaussian Extractor application.
 */

#include "interactive_mode.h"
#include "help_utils.h"
#include "input_gen/parameter_parser.h"
#include "utilities/command_system.h"
#include "utilities/config_manager.h"
#include "utilities/utils.h"
#include "utilities/version.h"
#include <atomic>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Windows-specific includes for TAB completion
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#endif

// External declarations for globals defined in main.cpp
extern std::atomic<bool> g_shutdown_requested;
extern ConfigManager     g_config_manager;

#ifdef _WIN32
std::string escape_for_ps(const std::string& s)
{
    std::string res;
    bool        needs_quotes = false;

    // Check if the string contains spaces or special characters that require quoting
    for (char c : s)
    {
        if (c == ' ' || c == ',' || c == ';' || c == '|' || c == '&' || c == '(' || c == ')' || c == '{' || c == '}')
        {
            needs_quotes = true;
            break;
        }
    }

    if (needs_quotes)
    {
        res += '"';
        for (char c : s)
        {
            // Escape PowerShell special characters with backtick
            if (c == '"' || c == '`' || c == '$')
            {
                res += '`';
                res += c;
            }
            else
            {
                res += c;
            }
        }
        res += '"';
    }
    else
    {
        for (char c : s)
        {
            // Escape only specific special characters
            if (c == '"' || c == '`' || c == '$')
            {
                res += '`';
                res += c;
            }
            else
            {
                res += c;
            }
        }
    }

    return res;
}

std::string map_to_windows_ps(const std::string& cmd)
{
    std::istringstream iss(cmd);
    std::string        command_word;
    iss >> command_word;
    std::string arguments;
    std::getline(iss, arguments);  // Get the rest of the line, including spaces
    // Trim leading space
    size_t arg_start = arguments.find_first_not_of(" \t");
    if (arg_start != std::string::npos)
    {
        arguments = arguments.substr(arg_start);
    }
    else
    {
        arguments.clear();
    }

    if (command_word == "rm")
    {
        return "Remove-Item " + arguments;
    }
    else if (command_word == "mv")
    {
        return "Move-Item " + arguments;
    }
    else if (command_word == "cp")
    {
        return "Copy-Item " + arguments;
    }
    else if (command_word == "less")
    {
        return "more " + arguments;
    }
    else if (command_word == "grep")
    {
        return "Select-String " + arguments;
    }
    else if (command_word == "head")
    {
        return "Get-Content " + arguments + " -Head 10";
    }
    else if (command_word == "tail")
    {
        return "Get-Content " + arguments + " -Tail 10";
    }
    else if (command_word == "find")
    {
        return "Get-ChildItem -Recurse " + arguments;
    }
    else if (command_word == "history")
    {
        return "Get-History " + arguments;
    }
    else if (command_word == "alias")
    {
        size_t eq_pos = arguments.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string name     = arguments.substr(0, eq_pos);
            size_t      name_end = name.find_last_not_of(" \t");
            if (name_end != std::string::npos)
            {
                name = name.substr(0, name_end + 1);
            }
            std::string value       = arguments.substr(eq_pos + 1);
            size_t      value_start = value.find_first_not_of(" \t");
            if (value_start != std::string::npos)
            {
                value = value.substr(value_start);
            }
            return "New-Alias " + name + " \"" + escape_for_ps(value) + "\"";
        }
        else if (arguments.empty())
        {
            return "Get-Alias";
        }
        else
        {
            return "Get-Alias " + arguments;
        }
    }
    else if (command_word == "export")
    {
        size_t eq_pos = arguments.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string var     = arguments.substr(0, eq_pos);
            size_t      var_end = var.find_last_not_of(" \t");
            if (var_end != std::string::npos)
            {
                var = var.substr(0, var_end + 1);
            }
            std::string value       = arguments.substr(eq_pos + 1);
            size_t      value_start = value.find_first_not_of(" \t");
            if (value_start != std::string::npos)
            {
                value = value.substr(value_start);
            }
            return "$env:" + var + " = \"" + escape_for_ps(value) + "\"";
        }
        else
        {
            return "$env:" + arguments;
        }
    }
    else if (command_word == "unset")
    {
        return "Remove-Item Env:" + arguments;
    }
    else if (command_word == "source")
    {
        return ". \"" + escape_for_ps(arguments) + "\"";
    }
    else if (command_word == "date")
    {
        return "Get-Date " + arguments;
    }
    else if (command_word == "time")
    {
        return "Get-Date -Format t " + arguments;
    }
    else if (command_word == "bash" || command_word == "sh")
    {
        return "pwsh " + arguments;
    }
    else if (command_word == "touch")
    {
        if (arguments.empty())
        {
            return "Write-Error 'touch: missing file operand'";
        }
        // Split arguments into individual paths
        std::vector<std::string> paths;
        std::istringstream       iss(arguments);
        std::string              path;
        while (iss >> path)
        {
            paths.push_back(escape_for_ps(path));
        }
        // Join paths with commas for PowerShell
        std::string path_list;
        for (size_t i = 0; i < paths.size(); ++i)
        {
            path_list += paths[i];
            if (i < paths.size() - 1)
            {
                path_list += ",";
            }
        }
        return "New-Item -ItemType File -Path " + path_list + " -Force";
    }
    else
    {
        return cmd;
    }
}
#endif

#ifdef _WIN32
namespace fs = std::filesystem;

/**
 * @brief TAB completion engine for Windows interactive mode
 */
class TabCompleter
{
public:
    TabCompleter()
    {
        // Initialize internal commands
        internal_commands = {"extract",
                             "done",
                             "errors",
                             "pcm",
                             "imode",
                             "check",
                             "high-kj",
                             "high-au",
                             "xyz",
                             "ci",
                             "help",
                             "exit",
                             "quit",
                             "--version",
                             "--config-help",
                             "--create-config",
                             "--show-config",
                             "--genci-params",
                             "--genci-all-params"};

        // Common shell commands for Windows
        shell_commands = {"cd",
                          "dir",
                          "pwd",
                          "mkdir",
                          "rmdir",
                          "copy",
                          "move",
                          "del",
                          "type",
                          "more",
                          "echo",
                          "cls",
                          "date",
                          "time",
                          "findstr",
                          "grep",
                          "where",
                          "set",
                          "path"};
    }

    // Case-insensitive string comparison for Windows
    static bool iequals(const std::string& a, const std::string& b)
    {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
            return std::tolower(a) == std::tolower(b);
        });
    }

    // Case-insensitive prefix check
    static bool istarts_with(const std::string& str, const std::string& prefix)
    {
        if (prefix.size() > str.size())
            return false;
        return std::equal(prefix.begin(), prefix.end(), str.begin(), [](char a, char b) {
            return std::tolower(a) == std::tolower(b);
        });
    }

    // Extract the current token being typed
    std::string extract_current_token(const std::string& input, size_t cursor_pos)
    {
        if (cursor_pos > input.length())
            cursor_pos = input.length();

        // Find start of current token
        size_t token_start = cursor_pos;
        while (token_start > 0 && input[token_start - 1] != ' ')
        {
            token_start--;
        }

        // Find end of current token
        size_t token_end = cursor_pos;
        while (token_end < input.length() && input[token_end] != ' ')
        {
            token_end++;
        }

        return input.substr(token_start, token_end - token_start);
    }

    // Get position of current token in the input
    size_t get_token_start_pos(const std::string& input, size_t cursor_pos)
    {
        if (cursor_pos > input.length())
            cursor_pos = input.length();

        size_t token_start = cursor_pos;
        while (token_start > 0 && input[token_start - 1] != ' ')
        {
            token_start--;
        }
        return token_start;
    }

    // Find all possible completions for a prefix
    std::vector<std::string> find_completions(const std::string& prefix, const std::string& full_input)
    {
        std::vector<std::string> matches;

        // Determine context - are we completing the first token (command) or subsequent tokens?
        std::istringstream iss(full_input);
        std::string        first_token;
        iss >> first_token;

        bool is_first_token = (full_input.find(' ') == std::string::npos) ||
                              (full_input.find(' ') > get_token_start_pos(full_input, prefix.length()));

        if (is_first_token)
        {
            // Complete commands
            add_command_completions(prefix, matches);
        }

        // Always add file/directory completions
        add_filesystem_completions(prefix, matches);

        // Remove duplicates and sort
        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());

        return matches;
    }

private:
    std::vector<std::string> internal_commands;
    std::vector<std::string> shell_commands;

    void add_command_completions(const std::string& prefix, std::vector<std::string>& matches)
    {
        // Check internal commands
        for (const auto& cmd : internal_commands)
        {
            if (istarts_with(cmd, prefix))
            {
                matches.push_back(cmd);
            }
        }

        // Check shell commands
        for (const auto& cmd : shell_commands)
        {
            if (istarts_with(cmd, prefix))
            {
                matches.push_back(cmd);
            }
        }
    }

    void add_filesystem_completions(const std::string& prefix, std::vector<std::string>& matches)
    {
        try
        {
            fs::path    prefix_path(prefix);
            fs::path    search_dir  = ".";
            std::string file_prefix = prefix;

            // If prefix contains a directory separator, split it
            if (prefix.find('\\') != std::string::npos || prefix.find('/') != std::string::npos)
            {
                if (prefix_path.has_parent_path())
                {
                    search_dir  = prefix_path.parent_path();
                    file_prefix = prefix_path.filename().string();
                }
            }

            // Enumerate files and directories
            if (fs::exists(search_dir) && fs::is_directory(search_dir))
            {
                for (const auto& entry : fs::directory_iterator(search_dir))
                {
                    std::string filename = entry.path().filename().string();

                    if (istarts_with(filename, file_prefix))
                    {
                        std::string completion;
                        if (search_dir != ".")
                        {
                            completion = (search_dir / filename).string();
                        }
                        else
                        {
                            completion = filename;
                        }

                        // Add trailing slash for directories
                        if (entry.is_directory())
                        {
                            completion += "\\";
                        }

                        // Handle spaces in filenames
                        if (completion.find(' ') != std::string::npos)
                        {
                            completion = "\"" + completion + "\"";
                        }

                        matches.push_back(completion);
                    }
                }
            }
        }
        catch (const std::exception&)
        {
            // Silently ignore filesystem errors
        }
    }
};
#endif

#ifdef _WIN32
/**
 * @brief Enhanced line reader with TAB completion for Windows
 */
class InteractiveLineReader
{
private:
    HANDLE                   hConsoleInput;
    HANDLE                   hConsoleOutput;
    DWORD                    original_mode;
    TabCompleter             completer;
    std::vector<std::string> history;
    size_t                   history_index;

public:
    InteractiveLineReader() : history_index(0)
    {
        hConsoleInput  = GetStdHandle(STD_INPUT_HANDLE);
        hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

        // Save original console mode
        GetConsoleMode(hConsoleInput, &original_mode);
    }

    ~InteractiveLineReader()
    {
        // Restore original console mode
        SetConsoleMode(hConsoleInput, original_mode);
    }

    std::string read_line_with_completion(const std::string& prompt)
    {
        std::cout << prompt;
        std::cout.flush();

        std::string              input;
        size_t                   cursor_pos = 0;
        std::vector<std::string> last_completions;
        size_t                   completion_index = 0;

        // Set console to raw mode for single character input
        DWORD raw_mode = original_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(hConsoleInput, raw_mode);

        while (true)
        {
            INPUT_RECORD inputRecord;
            DWORD        events;

            if (!ReadConsoleInput(hConsoleInput, &inputRecord, 1, &events))
            {
                break;
            }

            if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown)
            {

                WORD vkey = inputRecord.Event.KeyEvent.wVirtualKeyCode;
                char ch   = inputRecord.Event.KeyEvent.uChar.AsciiChar;

                if (vkey == VK_TAB)
                {
                    // TAB key pressed - perform completion
                    handle_tab_completion(input, cursor_pos, last_completions, completion_index, prompt);
                }
                else if (vkey == VK_RETURN)
                {
                    // Enter key - finish input
                    std::cout << std::endl;
                    break;
                }
                else if (vkey == VK_BACK)
                {
                    // Backspace
                    if (cursor_pos > 0)
                    {
                        input.erase(cursor_pos - 1, 1);
                        cursor_pos--;
                        redraw_line(prompt, input, cursor_pos);
                    }
                    last_completions.clear();
                }
                else if (vkey == VK_DELETE)
                {
                    // Delete key
                    if (cursor_pos < input.length())
                    {
                        input.erase(cursor_pos, 1);
                        redraw_line(prompt, input, cursor_pos);
                    }
                    last_completions.clear();
                }
                else if (vkey == VK_LEFT)
                {
                    // Left arrow
                    if (cursor_pos > 0)
                    {
                        cursor_pos--;
                        update_cursor_position(prompt, cursor_pos);
                    }
                }
                else if (vkey == VK_RIGHT)
                {
                    // Right arrow
                    if (cursor_pos < input.length())
                    {
                        cursor_pos++;
                        update_cursor_position(prompt, cursor_pos);
                    }
                }
                else if (vkey == VK_HOME)
                {
                    // Home key
                    cursor_pos = 0;
                    update_cursor_position(prompt, cursor_pos);
                }
                else if (vkey == VK_END)
                {
                    // End key
                    cursor_pos = input.length();
                    update_cursor_position(prompt, cursor_pos);
                }
                else if (vkey == VK_UP)
                {
                    // Up arrow - history navigation
                    if (history_index > 0)
                    {
                        history_index--;
                        input      = history[history_index];
                        cursor_pos = input.length();
                        redraw_line(prompt, input, cursor_pos);
                    }
                }
                else if (vkey == VK_DOWN)
                {
                    // Down arrow - history navigation
                    if (history_index < history.size() - 1)
                    {
                        history_index++;
                        input      = history[history_index];
                        cursor_pos = input.length();
                        redraw_line(prompt, input, cursor_pos);
                    }
                    else if (history_index == history.size() - 1)
                    {
                        history_index++;
                        input      = "";
                        cursor_pos = 0;
                        redraw_line(prompt, input, cursor_pos);
                    }
                }
                else if (ch >= 32 && ch < 127)
                {
                    // Regular character
                    input.insert(cursor_pos, 1, ch);
                    cursor_pos++;
                    redraw_line(prompt, input, cursor_pos);
                    last_completions.clear();
                }
            }
        }

        // Restore console mode
        SetConsoleMode(hConsoleInput, original_mode);

        // Add to history if not empty
        if (!input.empty())
        {
            history.push_back(input);
            history_index = history.size();
        }

        return input;
    }

private:
    void handle_tab_completion(std::string&              input,
                               size_t&                   cursor_pos,
                               std::vector<std::string>& last_completions,
                               size_t&                   completion_index,
                               const std::string&        prompt)
    {

        // Get the token to complete
        std::string token       = completer.extract_current_token(input, cursor_pos);
        size_t      token_start = completer.get_token_start_pos(input, cursor_pos);

        // If we're cycling through completions
        if (!last_completions.empty())
        {
            completion_index = (completion_index + 1) % last_completions.size();
            apply_completion(input, cursor_pos, token_start, token.length(), last_completions[completion_index]);
            redraw_line(prompt, input, cursor_pos);
            return;
        }

        // Find new completions
        std::vector<std::string> completions = completer.find_completions(token, input);

        if (completions.empty())
        {
            // No completions found - beep
            MessageBeep(0);
        }
        else if (completions.size() == 1)
        {
            // Single completion - apply it
            apply_completion(input, cursor_pos, token_start, token.length(), completions[0]);
            redraw_line(prompt, input, cursor_pos);
        }
        else
        {
            // Multiple completions - show them and start cycling
            show_completions(completions);
            std::cout << prompt << input;
            std::cout.flush();
            update_cursor_position(prompt, cursor_pos);

            last_completions = completions;
            completion_index = 0;
        }
    }

    void apply_completion(std::string&       input,
                          size_t&            cursor_pos,
                          size_t             token_start,
                          size_t             token_length,
                          const std::string& completion)
    {
        input.erase(token_start, token_length);
        input.insert(token_start, completion);

        // Add space after completion if it's not a directory
        if (!completion.empty() && completion.back() != '\\')
        {
            input.insert(token_start + completion.length(), " ");
            cursor_pos = token_start + completion.length() + 1;
        }
        else
        {
            cursor_pos = token_start + completion.length();
        }
    }

    void show_completions(const std::vector<std::string>& completions)
    {
        std::cout << std::endl;

        // Calculate column width
        size_t max_len = 0;
        for (const auto& c : completions)
        {
            max_len = std::max(max_len, c.length());
        }

        // Get console width
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
        int console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;

        // Display in columns
        size_t col_width = max_len + 2;
        size_t num_cols  = std::max(1, console_width / (int)col_width);

        for (size_t i = 0; i < completions.size(); i++)
        {
            std::cout << completions[i];

            // Pad to column width
            for (size_t j = completions[i].length(); j < col_width; j++)
            {
                std::cout << " ";
            }

            if ((i + 1) % num_cols == 0)
            {
                std::cout << std::endl;
            }
        }

        if (completions.size() % num_cols != 0)
        {
            std::cout << std::endl;
        }
    }

    void redraw_line(const std::string& prompt, const std::string& input, size_t cursor_pos)
    {
        // Clear current line
        std::cout << '\r';
        for (size_t i = 0; i < prompt.length() + input.length() + 5; i++)
        {
            std::cout << ' ';
        }

        // Redraw
        std::cout << '\r' << prompt << input;
        std::cout.flush();

        // Position cursor
        update_cursor_position(prompt, cursor_pos);
    }

    void update_cursor_position(const std::string& prompt, size_t cursor_pos)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);

        COORD pos;
        pos.X = (SHORT)(prompt.length() + cursor_pos);
        pos.Y = csbi.dwCursorPosition.Y;

        SetConsoleCursorPosition(hConsoleOutput, pos);
    }
};
#endif

/**
 * @brief Enhanced interactive command loop with TAB completion for Windows
 * @return Exit code when the interactive loop terminates
 */
int run_interactive_loop()
{
    std::cout << "\nGaussian Extractor Interactive Mode" << std::endl;
    std::cout << "Developed by Le Nhan Pham" << std::endl;
    std::cout << "https://github.com/lenhanpham/gaussian-extractor" << std::endl;
    std::cout << "\nType 'help' for available commands, 'exit' or 'quit' to exit." << std::endl;

#ifdef _WIN32
    // Windows with TAB completion
    std::cout << "Press TAB for command and file completion." << std::endl;

    InteractiveLineReader reader;

    while (true)
    {
        std::string line = reader.read_line_with_completion(">> ");
#else
    // Non-Windows systems - fallback to original implementation
    std::cout << ">> ";

    std::string line;
    while (std::getline(std::cin, line))
    {
#endif
        // Trim whitespace and carriage return
        size_t start = line.find_first_not_of(" \t\r");
        if (start != std::string::npos)
        {
            line.erase(0, start);
        }
        else
        {
            line.clear();
        }
        size_t end = line.find_last_not_of(" \t\r");
        if (end != std::string::npos)
        {
            line.erase(end + 1);
        }

        if (line.empty())
        {
#ifndef _WIN32
            std::cout << ">> ";
#endif
            continue;
        }

        if (line == "exit" || line == "quit")
        {
            std::cout << "Exiting Gaussian Extractor. Goodbye!" << std::endl;
#ifdef _WIN32
            std::cout << std::endl << "Press Enter to exit..." << std::endl;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
#endif
            return 0;
        }

        if (line == "help" || line == "--help" || line == "-h")
        {
            std::cout << "\nAvailable commands:" << std::endl;
            std::cout << "  extract           Extract thermodynamic data from log files" << std::endl;
            std::cout << "  done              Check and organize completed jobs" << std::endl;
            std::cout << "  errors            Check and organize failed jobs" << std::endl;
            std::cout << "  pcm               Check PCM convergence failures" << std::endl;
            std::cout << "  imode             Check jobs with imaginary frequencies" << std::endl;
            std::cout << "  check             Run all job checks" << std::endl;
            std::cout << "  high-kj           Calculate high-level energies in kJ/mol" << std::endl;
            std::cout << "  high-au           Calculate high-level energies in atomic units" << std::endl;
            std::cout << "  xyz               Extract coordinates to XYZ format" << std::endl;
            std::cout << "  ci                Create Gaussian input files from XYZ files" << std::endl;
            std::cout << "  help              Show this help message" << std::endl;
            std::cout << "  --version         Show version information" << std::endl;
            std::cout << "  --config-help     Show configuration help" << std::endl;
            std::cout << "  --create-config   Create default configuration file" << std::endl;
            std::cout << "  --show-config     Show current configuration" << std::endl;
            std::cout << "  --genci-params [type] [dir]  Generate parameter template" << std::endl;
            std::cout << "                                    (type defaults to general, dir defaults to current)"
                      << std::endl;
            std::cout << "  --genci-all-params [dir]    Generate all parameter templates in directory" << std::endl;
            std::cout << "                                    (optional dir, defaults to current directory)"
                      << std::endl;
            std::cout << "\nYou can use command-line options with any command." << std::endl;
            std::cout << "Example: extract -q -nt 4" << std::endl;
            std::cout << "\nYou can also use shell commands like 'cd', 'ls', 'pwd', etc." << std::endl;
#ifdef _WIN32
            std::cout << "\nPress TAB to auto-complete commands and filenames." << std::endl;
#endif
        }
        else if (line == "--version" || line == "-v")
        {
            std::cout << GaussianExtractor::get_version_info() << std::endl;
        }
        else if (line == "--config-help")
        {
            HelpUtils::print_config_help();
        }
        else if (line == "--create-config")
        {
            CommandParser::create_default_config();
        }
        else if (line == "--show-config")
        {
            g_config_manager.print_config_summary(true);
        }
        else if (line.find("--genci-params") == 0)
        {
            // Handle template generation
            std::vector<char*> args;
            try
            {
                // Split the line into arguments
                std::string        arg;
                std::istringstream iss_full(line);
                args.push_back(const_cast<char*>("gaussian_extractor"));  // Program name

                while (iss_full >> arg)
                {
                    char* cstr = new char[arg.size() + 1];
                    strcpy(cstr, arg.c_str());
                    args.push_back(cstr);
                }

                // Create a temporary context for parsing
                CommandContext temp_context;
                temp_context.command = CommandType::EXTRACT;  // Default

                // Parse the arguments (this will handle --genci-params)
                int    temp_argc = args.size();
                char** temp_argv = args.data();

                // Find the --genci-params argument and handle it
                for (int i = 1; i < temp_argc; ++i)
                {
                    std::string arg_str = temp_argv[i];
                    if (arg_str == "--genci-params")
                    {
                        std::string template_type = "";   // Default to general template
                        std::string directory     = ".";  // Default

                        // Check for optional template type
                        if (++i < temp_argc && temp_argv[i][0] != '-')
                        {
                            template_type = temp_argv[i];
                        }
                        else
                        {
                            --i;  // Reset if no type provided
                        }

                        // Check for optional directory
                        if (++i < temp_argc && temp_argv[i][0] != '-')
                        {
                            directory = temp_argv[i];
                        }
                        else
                        {
                            --i;  // Reset if no directory provided
                        }

                        // Generate the template
                        ParameterParser       parser;
                        std::filesystem::path dir_path(directory);
                        std::filesystem::path abs_dir_path = std::filesystem::absolute(dir_path);

                        if (template_type.empty() || template_type == "general" || template_type == "ci_parameters")
                        {
                            // Generate general template
                            std::filesystem::path base_path  = dir_path / "ci_parameters.params";
                            std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
                            if (parser.generateGeneralTemplate(final_path.string()))
                            {
                                std::cout << "General template generated successfully: " << final_path.string()
                                          << std::endl;
                            }
                            else
                            {
                                std::cerr << "Failed to generate general template" << std::endl;
                            }
                        }
                        else
                        {
                            // Generate specific template
                            std::filesystem::path base_path  = dir_path / (template_type + ".params");
                            std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
                            if (parser.generateTemplate(template_type, final_path.string()))
                            {
                                std::cout << "Template generated successfully: " << final_path.string() << std::endl;
                            }
                            else
                            {
                                std::cerr << "Failed to generate template for: " << template_type << std::endl;
                            }
                        }
                    }

                    // Clean up allocated strings
                    for (size_t i = 1; i < args.size(); ++i)
                    {
                        delete[] args[i];
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error generating template: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Unknown error occurred while generating template" << std::endl;
            }
        }
        else if (line.find("--genci-all-params") == 0)
        {
            // Handle all templates generation
            std::vector<char*> args;
            try
            {
                // Split the line into arguments
                std::string        arg;
                std::istringstream iss_full(line);
                args.push_back(const_cast<char*>("gaussian_extractor"));  // Program name

                while (iss_full >> arg)
                {
                    char* cstr = new char[arg.size() + 1];
                    strcpy(cstr, arg.c_str());
                    args.push_back(cstr);
                }

                // Create a temporary context for parsing
                CommandContext temp_context;
                temp_context.command = CommandType::EXTRACT;  // Default

                // Parse the arguments (this will handle --genci-all-params)
                int    temp_argc = args.size();
                char** temp_argv = args.data();

                // Find the --genci-all-params argument and handle it
                for (int i = 1; i < temp_argc; ++i)
                {
                    std::string arg_str = temp_argv[i];
                    if (arg_str == "--genci-all-params")
                    {
                        std::string directory = ".";  // Default

                        // Check for optional directory
                        if (++i < temp_argc && temp_argv[i][0] != '-')
                        {
                            directory = temp_argv[i];
                        }
                        else
                        {
                            --i;  // Reset if no directory provided
                        }

                        // Generate all templates
                        ParameterParser parser;
                        if (parser.generateAllTemplates(directory))
                        {
                            std::filesystem::path abs_dir_path = std::filesystem::absolute(directory);
                            std::cout << "All templates generated successfully in: " << abs_dir_path.string()
                                      << std::endl;
                        }
                        else
                        {
                            std::filesystem::path abs_dir_path = std::filesystem::absolute(directory);
                            std::cerr << "Failed to generate templates in: " << abs_dir_path.string() << std::endl;
                        }
                    }
                }

                // Clean up allocated strings
                for (size_t i = 1; i < args.size(); ++i)
                {
                    delete[] args[i];
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error generating templates: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Unknown error occurred while generating templates" << std::endl;
            }
        }
        // Handle help commands with arguments or command --help
        else if (line.find("help ") == 0 || line.find("--help ") == 0 || line.find(" --help") != std::string::npos)
        {
            std::string help_arg;
            if (line.find("help ") == 0)
            {
                help_arg = line.substr(5);  // Remove "help "
            }
            else if (line.find("--help ") == 0)
            {
                help_arg = line.substr(7);  // Remove "--help "
            }
            else
            {
                // Handle command --help pattern (e.g., "ci --help")
                std::istringstream iss(line);
                iss >> help_arg;  // Get the command before --help
                std::string remaining;
                std::getline(iss, remaining);
                if (remaining.find("--help") == std::string::npos)
                {
                    help_arg.clear();  // Not a valid --help command
                }
            }

            // Trim whitespace
            size_t start = help_arg.find_first_not_of(" \t\r");
            if (start != std::string::npos)
            {
                help_arg = help_arg.substr(start);
            }
            size_t end = help_arg.find_last_not_of(" \t\r");
            if (end != std::string::npos)
            {
                help_arg = help_arg.substr(0, end + 1);
            }

            // Map command names to CommandType
            static const std::map<std::string, CommandType> command_map = {{"extract", CommandType::EXTRACT},
                                                                           {"done", CommandType::CHECK_DONE},
                                                                           {"errors", CommandType::CHECK_ERRORS},
                                                                           {"pcm", CommandType::CHECK_PCM},
                                                                           {"imode", CommandType::CHECK_IMAGINARY},
                                                                           {"check", CommandType::CHECK_ALL},
                                                                           {"high-kj", CommandType::HIGH_LEVEL_KJ},
                                                                           {"high-au", CommandType::HIGH_LEVEL_AU},
                                                                           {"xyz", CommandType::EXTRACT_COORDS},
                                                                           {"ci", CommandType::CREATE_INPUT}};

            auto it = command_map.find(help_arg);
            if (it != command_map.end())
            {
                HelpUtils::print_command_help(it->second);
            }
            else if (!help_arg.empty())
            {
                std::cout << "Unknown command: " << help_arg << std::endl;
                std::cout << "Type 'help' for a list of available commands." << std::endl;
            }
            else
            {
                // Just "help" or "--help" without arguments
                HelpUtils::print_help();
            }
        }
        else if (is_shell_command(line))
        {
            // Handle directory commands natively
            if (is_directory_command(line))
            {
                int result = execute_directory_command(line);
                if (result != 0)
                {
                    std::cout << "Directory command failed with code: " << result << std::endl;
                }
            }
            // Handle utility commands specially
            else if (is_utility_command(line))
            {
                int result = execute_utility_command(line);
                if (result != 0)
                {
                    std::cout << "Utility command failed with code: " << result << std::endl;
                }
            }
            else
            {
                // Execute as shell command with PowerShell mapping on Windows
#ifdef _WIN32
                std::string mapped = map_to_windows_ps(line);
                // Apply escaping only for the outer command
                std::string shell_cmd = "powershell -NoProfile -Command \"" + mapped + "\" 2>&1";
#else
                std::string shell_cmd = line;
#endif
                // Debug: Print the command being executed
                // std::cout << "Executing: " << shell_cmd << std::endl;
                int result = std::system(shell_cmd.c_str());
                if (result != 0)
                {
                    std::cout << "Command exited with code: " << result << std::endl;
                }
            }
        }
        else
        {
            // Check if this could be a valid Gaussian command
            std::istringstream iss(line);
            std::string        first_arg;
            iss >> first_arg;

            // Check for invalid combinations of commands with template generation
            if ((first_arg == "ci" || first_arg == "--create-input") &&
                (line.find("--genci-params") != std::string::npos ||
                 line.find("--genci-all-params") != std::string::npos))
            {
                std::cout << "Error: Template generation commands (--genci-params, --genci-all-params) "
                          << "cannot be combined with other commands." << std::endl;
                std::cout << "Use them as standalone commands: --genci-params [type] [dir] or "
                          << "--genci-all-params [dir]" << std::endl;
            }
            else if (line.find("--genci-params") != std::string::npos ||
                     line.find("--genci-all-params") != std::string::npos)
            {
                // Handle template generation commands as standalone
                if (line.find("--genci-params") != std::string::npos)
                {
                    // Extract arguments for --genci-params
                    std::vector<std::string> tokens;
                    std::istringstream       iss_template(line);
                    std::string              token;
                    while (iss_template >> token)
                    {
                        tokens.push_back(token);
                    }

                    std::string template_type = "opt_freq";  // Default
                    std::string directory     = ".";         // Default

                    // Find --genci-params and parse arguments
                    for (size_t i = 0; i < tokens.size(); ++i)
                    {
                        if (tokens[i] == "--genci-params")
                        {
                            if (i + 1 < tokens.size() && tokens[i + 1][0] != '-')
                            {
                                template_type = tokens[i + 1];
                                if (i + 2 < tokens.size() && tokens[i + 2][0] != '-')
                                {
                                    directory = tokens[i + 2];
                                }
                            }
                            break;
                        }
                    }

                    // Generate the template
                    ParameterParser       parser;
                    std::filesystem::path dir_path(directory);
                    std::filesystem::path abs_dir_path = std::filesystem::absolute(dir_path);

                    if (template_type == "general" || template_type == "ci_parameters")
                    {
                        // Generate general template
                        std::filesystem::path file_path = dir_path / "ci_parameters.params";
                        if (parser.generateGeneralTemplate(file_path.string()))
                        {
                            std::cout << "General template generated successfully: " << file_path.string() << std::endl;
                        }
                        else
                        {
                            std::cerr << "Failed to generate general template" << std::endl;
                        }
                    }
                    else
                    {
                        // Generate specific template
                        std::filesystem::path file_path = dir_path / (template_type + ".params");
                        if (parser.generateTemplate(template_type, file_path.string()))
                        {
                            std::cout << "Template generated successfully: " << file_path.string() << std::endl;
                        }
                        else
                        {
                            std::cerr << "Failed to generate template for: " << template_type << std::endl;
                        }
                    }
                }
                else if (line.find("--genci-all-params") != std::string::npos)
                {
                    // Extract arguments for --genci-all-params
                    std::vector<std::string> tokens;
                    std::istringstream       iss_template(line);
                    std::string              token;
                    while (iss_template >> token)
                    {
                        tokens.push_back(token);
                    }

                    std::string directory = ".";  // Default

                    // Find --genci-all-params and parse arguments
                    for (size_t i = 0; i < tokens.size(); ++i)
                    {
                        if (tokens[i] == "--genci-all-params")
                        {
                            if (i + 1 < tokens.size() && tokens[i + 1][0] != '-')
                            {
                                directory = tokens[i + 1];
                            }
                            break;
                        }
                    }

                    // Generate all templates
                    ParameterParser parser;
                    if (parser.generateAllTemplates(directory))
                    {
                        std::filesystem::path abs_dir_path = std::filesystem::absolute(directory);
                        std::cout << "All templates generated successfully in: " << abs_dir_path.string() << std::endl;
                    }
                    else
                    {
                        std::filesystem::path abs_dir_path = std::filesystem::absolute(directory);
                        std::cerr << "Failed to generate templates in: " << abs_dir_path.string() << std::endl;
                    }
                }
            }
            else
            {
                // List of valid Gaussian commands
                static const std::vector<std::string> valid_commands = {"extract",
                                                                        "done",
                                                                        "errors",
                                                                        "pcm",
                                                                        "imode",
                                                                        "--imaginary",
                                                                        "check",
                                                                        "high-kj",
                                                                        "--high-level-kj",
                                                                        "high-au",
                                                                        "--high-level-au",
                                                                        "xyz",
                                                                        "--extract-coord",
                                                                        "ci",
                                                                        "--create-input"};

                bool is_valid_gaussian_command = false;
                for (const auto& cmd : valid_commands)
                {
                    if (first_arg == cmd)
                    {
                        is_valid_gaussian_command = true;
                        break;
                    }
                }

                if (is_valid_gaussian_command)
                {
                    // Parse and execute as Gaussian command
                    std::vector<char*> args;
                    try
                    {
                        // Split the line into arguments
                        std::string        arg;
                        std::istringstream iss_full(line);
                        args.push_back(const_cast<char*>("gaussian_extractor"));  // Program name

                        while (iss_full >> arg)
                        {
                            char* cstr = new char[arg.size() + 1];
                            strcpy(cstr, arg.c_str());
                            args.push_back(cstr);
                        }

                        if (args.size() > 1)
                        {
                            // Parse the command
                            CommandContext context = CommandParser::parse(args.size(), args.data());

                            // Show warnings if any
                            if (!context.warnings.empty() && !context.quiet)
                            {
                                for (const auto& warning : context.warnings)
                                {
                                    std::cerr << warning << std::endl;
                                }
                                std::cerr << std::endl;
                            }

                            // Execute based on command type
                            int result = 0;
                            switch (context.command)
                            {
                                case CommandType::EXTRACT:
                                    result = execute_extract_command(context);
                                    break;
                                case CommandType::CHECK_DONE:
                                    result = execute_check_done_command(context);
                                    break;
                                case CommandType::CHECK_ERRORS:
                                    result = execute_check_errors_command(context);
                                    break;
                                case CommandType::CHECK_PCM:
                                    result = execute_check_pcm_command(context);
                                    break;
                                case CommandType::CHECK_IMAGINARY:
                                    result = execute_check_imaginary_command(context);
                                    break;
                                case CommandType::CHECK_ALL:
                                    result = execute_check_all_command(context);
                                    break;
                                case CommandType::HIGH_LEVEL_KJ:
                                    result = execute_high_level_kj_command(context);
                                    break;
                                case CommandType::HIGH_LEVEL_AU:
                                    result = execute_high_level_au_command(context);
                                    break;
                                case CommandType::EXTRACT_COORDS:
                                    result = execute_extract_coords_command(context);
                                    break;
                                case CommandType::CREATE_INPUT:
                                    result = execute_create_input_command(context);
                                    break;
                                default:
                                    std::cerr << "Unknown command" << std::endl;
                                    result = 1;
                            }

                            if (result != 0)
                            {
                                std::cerr << "Command failed with exit code: " << result << std::endl;
                            }
                        }
                        else
                        {
                            std::cout << "Please enter a command. Type 'help' for available commands." << std::endl;
                        }

                        // Clean up allocated strings
                        for (size_t i = 1; i < args.size(); ++i)
                        {
                            delete[] args[i];
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Error parsing command: " << e.what() << std::endl;
                    }
                    catch (...)
                    {
                        std::cerr << "Unknown error occurred while parsing command" << std::endl;
                    }
                }
                else
                {
                    // Unknown command
                    std::cout << "Unknown command: " << first_arg << std::endl;
                    std::cout << "Type 'help' for a list of available commands." << std::endl;
                }
            }
        }

#ifndef _WIN32
        std::cout << ">> ";
#endif
    }

    // Loop exited (EOF or error)
    std::cout << "Exiting Gaussian Extractor. Goodbye!" << std::endl;
#ifdef _WIN32
    std::cout << std::endl << "Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
#endif

    return 0;
}

bool is_shell_command(const std::string& cmd)
{
    // Common shell commands that should be executed directly
    static const std::vector<std::string> shell_commands = {
        "cd",    "ls",     "dir",  "pwd",   "mkdir",  "rmdir", "cp",   "mv",    "rm",   "cat",    "more",
        "less",  "head",   "tail", "grep",  "find",   "which", "echo", "date",  "time", "touch",  "history",
        "alias", "export", "set",  "unset", "source", "bash",  "sh",   "zsh",   "fish", "python", "python3",
        "pip",   "pip3",   "git",  "make",  "cmake",  "gcc",   "g++",  "clang", "vim",  "nano",   "emacs",
        "code",  "subl",   "atom", "del",   "clear",  "cls",   "exit"};

    // Check if command starts with any shell command
    for (const auto& shell_cmd : shell_commands)
    {
        if (cmd.find(shell_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = shell_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

bool is_directory_command(const std::string& cmd)
{
    // Commands that change the current working directory or show it
    static const std::vector<std::string> dir_commands = {"cd", "pushd", "popd", "pwd", "ls", "dir"};

    // Check if command starts with any directory command
    for (const auto& dir_cmd : dir_commands)
    {
        if (cmd.find(dir_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = dir_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

bool is_utility_command(const std::string& cmd)
{
    // Commands that provide utility functions and should be handled specially
    static const std::vector<std::string> utility_commands = {"which"};

    // Check if command starts with any utility command
    for (const auto& util_cmd : utility_commands)
    {
        if (cmd.find(util_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = util_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

int execute_utility_command(const std::string& cmd)
{
    try
    {
        if (cmd == "which")
        {
            std::cout << "which: missing argument" << std::endl;
            std::cout << "Usage: which <command>" << std::endl;
            return 1;
        }
        else if (cmd.find("which ") == 0)
        {
            std::string program = cmd.substr(6);
            // Trim whitespace
            size_t start = program.find_first_not_of(" \t\r");
            if (start != std::string::npos)
            {
                program = program.substr(start);
            }
            size_t end = program.find_last_not_of(" \t\r");
            if (end != std::string::npos)
            {
                program = program.substr(0, end + 1);
            }

            if (program.empty())
            {
                std::cout << "which: missing argument" << std::endl;
                return 1;
            }

            // Get PATH environment variable
            const char* path_env = std::getenv("PATH");
            if (!path_env)
            {
                std::cout << "which: PATH environment variable not found" << std::endl;
                return 1;
            }

            std::string              path_str = path_env;
            std::vector<std::string> paths;

            // Split PATH by delimiter
#ifdef _WIN32
            const char delimiter = ';';
#else
            const char delimiter = ':';
#endif

            size_t      pos = 0;
            std::string token;
            while ((pos = path_str.find(delimiter)) != std::string::npos)
            {
                token = path_str.substr(0, pos);
                if (!token.empty())
                {
                    paths.push_back(token);
                }
                path_str.erase(0, pos + 1);
            }
            if (!path_str.empty())
            {
                paths.push_back(path_str);
            }

            // Search for the program in each path
            for (const auto& path : paths)
            {
                std::filesystem::path full_path = std::filesystem::path(path) / program;

                // Try with .exe extension on Windows
#ifdef _WIN32
                std::filesystem::path exe_path = full_path;
                exe_path += ".exe";
                if (std::filesystem::exists(exe_path) && std::filesystem::is_regular_file(exe_path))
                {
                    std::cout << exe_path.string() << std::endl;
                    return 0;
                }
#endif

                // Try without extension
                if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path))
                {
                    std::cout << full_path.string() << std::endl;
                    return 0;
                }
            }

            std::cout << program << " not found in PATH" << std::endl;
            return 1;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "which: " << e.what() << std::endl;
        return 1;
    }

    return 1;  // Unknown utility command
}

int execute_directory_command(const std::string& cmd)
{
    try
    {
        if (cmd == "pwd")
        {
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd == "ls" || cmd == "dir")
        {
            // List directory contents
            for (const auto& entry : std::filesystem::directory_iterator("."))
            {
                std::cout << entry.path().filename().string();
                if (std::filesystem::is_directory(entry.path()))
                {
                    std::cout << "/";
                }
                std::cout << std::endl;
            }
            return 0;
        }
        else if (cmd.find("cd ") == 0 || cmd == "cd")
        {
            std::string path = ".";
            if (cmd.length() > 3)
            {
                path = cmd.substr(3);
                // Trim leading/trailing whitespace
                size_t start = path.find_first_not_of(" \t");
                if (start != std::string::npos)
                {
                    path = path.substr(start);
                }
                size_t end = path.find_last_not_of(" \t");
                if (end != std::string::npos)
                {
                    path = path.substr(0, end + 1);
                }
            }

            std::filesystem::current_path(path);
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd.find("pushd ") == 0)
        {
            // For simplicity, treat pushd like cd (full implementation would need a directory stack)
            std::string path = cmd.substr(6);
            // Trim whitespace
            size_t start = path.find_first_not_of(" \t");
            if (start != std::string::npos)
            {
                path = path.substr(start);
            }
            size_t end = path.find_last_not_of(" \t");
            if (end != std::string::npos)
            {
                path = path.substr(0, end + 1);
            }

            std::filesystem::current_path(path);
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd == "popd")
        {
            // For simplicity, just go to parent directory
            std::filesystem::current_path("..");
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Directory command failed: " << e.what() << std::endl;
        return 1;
    }

    return 1;  // Unknown directory command
}
