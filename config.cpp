#include "config.h"

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";

    // 对 optarg 的有效性检查，避免非法输入导致的未定义行为。
    auto validate_and_convert = [](const char* optarg, const std::string& option_name) -> int {
        if (!optarg || !std::isdigit(optarg[0])) {
            std::cerr << "Invalid value for option: " << option_name << ". Expected a number." << std::endl;
            return -1; // 返回无效值
        }
        return atoi(optarg);
    };

    while ((opt = getopt(argc, argv, str)) != -1) {
        int value = -1; // 初始化为无效值
        switch (opt) {
        case 'p':
            value = validate_and_convert(optarg, "-p (port)");
            if (value != -1) PORT = value;
            break;
        case 'l':
            value = validate_and_convert(optarg, "-l (log write)");
            if (value != -1) LOGWrite = value;
            break;
        case 'm':
            value = validate_and_convert(optarg, "-m (trigger mode)");
            if (value != -1) TRIGMode = value;
            break;
        case 'o':
            value = validate_and_convert(optarg, "-o (linger option)");
            if (value != -1) OPT_LINGER = value;
            break;
        case 's':
            value = validate_and_convert(optarg, "-s (sql num)");
            if (value != -1) sql_num = value;
            break;
        case 't':
            value = validate_and_convert(optarg, "-t (thread num)");
            if (value != -1) thread_num = value;
            break;
        case 'c':
            value = validate_and_convert(optarg, "-c (close log)");
            if (value != -1) close_log = value;
            break;
        case 'a':
            value = validate_and_convert(optarg, "-a (actor model)");
            if (value != -1) actor_model = value;
            break;
        default:
            std::cerr << "Unknown option: " << static_cast<char>(opt) << std::endl;
            break;
        }
    }
}