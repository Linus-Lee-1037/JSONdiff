#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <oneTBB/include/tbb/concurrent_queue.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <stdexcept>
#include <regex>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <cmath>
#include <cctype>
using namespace rapidjson;
using namespace std;

const std::string NOTHING = "__NON_EXIST__";
const std::string EVENT_PAIR = "pairs";
const std::string EVENT_OBJECT_REMOVE = "object:remove";
const std::string EVENT_OBJECT_ADD = "object:add";
const std::string EVENT_ARRAY_REMOVE = "array:remove";
const std::string EVENT_ARRAY_ADD = "array:add";
const std::string EVENT_VALUE_CHANGE = "value_changes";

namespace Linus
{
    namespace jsondiff
    {
        std::string ValueToString(const rapidjson::Value& value);
        std::vector<std::string> KeysFromObject(const rapidjson::Value& value);
        class TreeLevel
        {
            public:
                static const std::string empty_string;
                const rapidjson::Value& left;
                const rapidjson::Value& right;
                const std::string& left_path;
                const std::string& right_path;
                const std::string& up;

                TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input,\
                 const std::string& path_left, const std::string& path_right, const std::string& up_level);
                TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input);

                int get_type();
                std::string to_info();
                std::string get_key();
        };
        class JsonDiffer
        {
            public:
                const double SIMILARITY_THRESHOLD;
                const rapidjson::Value& left;
                const rapidjson::Value& right;
                std::map<std::string, double> cache;
                std::map<std::string, std::vector<std::string>> records;
                bool advanced_mode;
                int num_thread;
                std::mutex cache_mutex;
                JsonDiffer(const rapidjson::Value& left_input, const rapidjson::Value& right_input, bool advanced, double similarity_threshold, int thread_count);
                void report(std::string event, Linus::jsondiff::TreeLevel level);
                std::map<std::string, std::vector<std::string>> to_info();
                double compare_array(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_array_fast(Linus::jsondiff::TreeLevel level, bool drill);
                int get_type(const rapidjson::Value& input);
                void parallel_diff_level(std::queue<std::pair<unsigned int, unsigned int>>& work_queue, std::vector<std::vector<double>>& dp, Linus::jsondiff::TreeLevel& level, std::mutex& work_queue_mutex, std::mutex& dp_mutex);
                std::map<unsigned int, unsigned int> parallel_LCS(Linus::jsondiff::TreeLevel level);
                std::map<unsigned int, unsigned int> LCS(Linus::jsondiff::TreeLevel level, bool drill);
                std::vector<double> NWScore(bool reverse, Linus::jsondiff::TreeLevel level, bool drill, std::vector<int>& type_left, unsigned int sleft, unsigned int eleft, std::vector<int>& type_right, unsigned int sright, unsigned int eright);
                std::map<unsigned int, unsigned int> Hirschberg(Linus::jsondiff::TreeLevel level, bool drill, std::vector<int>& type_left, unsigned int sleft, unsigned int eleft, std::vector<int>& type_right, unsigned int sright, unsigned int eright);
                std::map<unsigned int, unsigned int> Hirschberg_starter(Linus::jsondiff::TreeLevel level);
                double compare_array_advanced(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_object(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_Int(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_Double(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_String(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_Bool(Linus::jsondiff::TreeLevel level, bool drill);
                double _diff_level(Linus::jsondiff::TreeLevel level, bool drill);
                double diff_level(Linus::jsondiff::TreeLevel level, bool drill);
                bool diff();
        };
        class BottomUpLCS
        {
            public:
                Linus::jsondiff::JsonDiffer& jsondiffer;
                static const std::string empty_string;
                std::map<unsigned int, std::vector<std::string>> left_nested_array;
                std::map<unsigned int, std::vector<std::string>> right_nested_array;
                std::map<std::string, double> history;
                Linus::jsondiff::TreeLevel level;
                BottomUpLCS(Linus::jsondiff::TreeLevel level_, Linus::jsondiff::JsonDiffer& differ);
                int get_type(const rapidjson::Value& input);
                void locate_array(const rapidjson::Value& tree, const std::string& path, unsigned int layer, bool lr);
                bool isUInt(const std::string& s);
                bool path_same(const std::string& left_path, const std::string& right_path);
                std::map<unsigned int, unsigned int> bu_computing();
                void LCS(const std::string& left_path, const std::string& right_path);
                double object_compare(Linus::jsondiff::TreeLevel level_, const std::string& left_path, const std::string& right_path);
        };
    }
}