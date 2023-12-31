#include "document.h"
using namespace std;
using namespace Linus::jsondiff;
using namespace rapidjson;

std::string Linus::jsondiff::ValueToString(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

std::vector<std::string> Linus::jsondiff::KeysFromObject(const rapidjson::Value& value)
{
    std::vector<std::string> keys;
    for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr)
    {
        keys.push_back(itr->name.GetString());
    }
    return keys;
}

Linus::jsondiff::TreeLevel::TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input, std::string path_left, std::string path_right, std::string up_level) : left(left_input), right(right_input), left_path(path_left), right_path(path_right), up(up_level)
{
    
}

std::string Linus::jsondiff::TreeLevel::get_type()
{
    if (left.IsObject() && right.IsObject())
    {
        return "Object";
    }
    else if (left.IsArray() && right.IsArray())
    {
        return "Array";
    }
    else if (left.IsString() && right.IsString())
    {
        return "String";
    }
    else if (left.IsNumber() && right.IsNumber())
    {
        return "Number";
    }
    else if (left.IsBool() && right.IsBool())
    {
        return "Bool";
    }
    else if (left.IsNull() && right.IsNull())
    {
        return "Null";
    }
    else
    {
        return "TypeMismatch";
    }
}

std::string Linus::jsondiff::TreeLevel::to_info()
{
    std::ostringstream info;
    info << "{\"left\":" << ValueToString(left)
           << ",\"right\":" << ValueToString(right)
           << ",\"left_path\":" << left_path
           << ",\"right_path\":" << right_path << "}";
    return info.str();
}

std::string Linus::jsondiff::TreeLevel::get_key()
{
    std::ostringstream key;
    key << left_path << "/" << right_path;
    return key.str();
}

Linus::jsondiff::JsonDiffer::JsonDiffer(const rapidjson::Value& left_input, const rapidjson::Value& right_input, bool advanced, double similarity_threshold, int thread_count) :left(left_input), right(right_input), advanced_mode(advanced), SIMILARITY_THRESHOLD(similarity_threshold), num_thread(thread_count)
{

}

void Linus::jsondiff::JsonDiffer::report(std::string event, Linus::jsondiff::TreeLevel level)
{
    records[event].push_back(level.to_info());
}

std::map<std::string, std::vector<std::string>> Linus::jsondiff::JsonDiffer::to_info()
{
    return records;
}

double Linus::jsondiff::JsonDiffer::compare_array(Linus::jsondiff::TreeLevel level, bool drill)
{
    double score;
    if (std::max(level.left.Size(), level.right.Size()) == 0)
    {
        return 1;
    }
    if (advanced_mode)
    {
        return compare_array_advanced(level, drill);
    }
    else
    {
        return compare_array_fast(level, drill);
    }
}

double Linus::jsondiff::JsonDiffer::compare_array_fast(Linus::jsondiff::TreeLevel level, bool drill)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    unsigned int min_len = std::min(len_left, len_right);
    unsigned int max_len = std::max(len_left, len_right);

    double total_score = 0;
    double score_;
    for (unsigned int index = 0; index < min_len; ++index)
    {
        Linus::jsondiff::TreeLevel level_(level.left[index], level.right[index], level.left_path + "[" + std::to_string(index) + "]", level.right_path + "[" + std::to_string(index) + "]", level.left_path);
        score_ = diff_level(level_, drill);
        total_score += score_;
    }

    rapidjson::Value emptyValue("");
    const rapidjson::Value& emptyRef = emptyValue;
    for (unsigned int index = min_len; index < len_left; ++index)
    {
        if (!drill) 
        {
            Linus::jsondiff::TreeLevel level_(level.left[index], emptyRef, level.left_path + "[" + std::to_string(index) + "]", "", level.left_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_REMOVE, level_);
        }
    }
    for (unsigned int index = min_len; index < len_right; ++index)
    {
        if (!drill) 
        {
            Linus::jsondiff::TreeLevel level_(emptyRef, level.right[index], "", level.right_path + "[" + std::to_string(index) + "]", level.right_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_ADD, level_);
        }
    }
    return total_score / max_len;
}

void Linus::jsondiff::JsonDiffer::parallel_diff_level(std::queue<std::pair<unsigned int, unsigned int>>& work_queue, std::vector<std::vector<double>>& dp, Linus::jsondiff::TreeLevel& level, std::mutex& work_queue_mutex, std::mutex& dp_mutex)
{
    bool ctn = true;
    int volumn = 6;
    while (ctn)
    {
        std::vector<std::pair<unsigned int, unsigned int>> task_list(volumn);
        int count = 0;
        {
            std::lock_guard<std::mutex> lock(work_queue_mutex);
            for (int i = 0; i < volumn; ++i)
            {
                if (work_queue.empty())
                {
                    ctn = false;
                    break;
                }
                task_list[i] = work_queue.front();
                work_queue.pop();
                ++count;
            }
            //std::cout << std::to_string(task.first) << ":" << std::to_string(task.second) << std::endl;
        }
        std::vector<double> score_(volumn);
        for (int i = 0; i < count; ++i)
        {
            Linus::jsondiff::TreeLevel level_(level.left[task_list[i].first], level.right[task_list[i].second], level.left_path + "[" + std::to_string(task_list[i].first) + "]", level.right_path + "[" + std::to_string(task_list[i].second) + "]", level.left_path);
            score_[i] = _diff_level(level_, true);
        }
        {
            std::lock_guard<std::mutex> lock(dp_mutex);
            for (int i = 0; i < count; ++i)
            {
                dp[task_list[i].first + 1][task_list[i].second + 1] = score_[i];
            }
        }
    }
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::parallel_LCS(Linus::jsondiff::TreeLevel level)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));
    std::queue<std::pair<unsigned int, unsigned int>> work_queue;

    //auto pLstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < len_left; ++i)
    {
        for (unsigned int j = 0; j < len_right; ++j)
        {
            work_queue.push(std::make_pair(i, j));
        }
    }

    //auto pLfinish = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> pLelapsed = pLfinish - pLstart;
    //std::cout << "Building worklist: " << pLelapsed.count() << " s\n";

    std::mutex work_queue_mutex;
    std::mutex dp_mutex;
    std::vector<std::thread> threads;

    //pLstart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_thread; ++i)
    {
        threads.push_back(std::thread(&Linus::jsondiff::JsonDiffer::parallel_diff_level, this, std::ref(work_queue), std::ref(dp), std::ref(level), std::ref(work_queue_mutex), std::ref(dp_mutex)));
    }

    //pLfinish = std::chrono::high_resolution_clock::now();
    //pLelapsed = pLfinish - pLstart;
    //std::cout << "Allocating threads: " << pLelapsed.count() << " s\n";

    for (auto& thread : threads)
    {
        thread.join();
    }

    //pLfinish = std::chrono::high_resolution_clock::now();
    //pLelapsed = pLfinish - pLstart;
    //std::cout << "Parallel computing scores: " << pLelapsed.count() << " s\n";

    //output dp table
    /*std::cout << "Before reconstruction" << std::endl;
    for (unsigned int i = 0; i <= len_left; ++i)
    {
        for (unsigned int j = 0; j <= len_right; ++j)
        {
            std::cout << dp[i][j] << " ";
        }
        std::cout << std::endl;
    }*/
    
    //pLstart = std::chrono::high_resolution_clock::now();

    //reconstruct dp table according to the result of parallel computing
    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            if (dp[i][j] > 0)
            {
                dp[i][j] += dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    //output dp table
    /*std::cout << "After reconstruction" << std::endl;
    for (unsigned int i = 0; i <= len_left; ++i)
    {
        for (unsigned int j = 0; j <= len_right; ++j)
        {
            std::cout << dp[i][j] << " ";
        }
        std::cout << std::endl;
    }*/
    
    //pLfinish = std::chrono::high_resolution_clock::now();
    //pLelapsed = pLfinish - pLstart;
    //std::cout << "Reconstructing of dp table: " << pLelapsed.count() << " s\n";

    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;

    //pLstart = std::chrono::high_resolution_clock::now();

    while (i > 0 && j > 0)
    {
        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
        double score_ = diff_level(level_, true);
        if (score_ > SIMILARITY_THRESHOLD)
        {
            pair_list[i - 1] = j - 1;
            --i;
            --j;
        }
        else if (dp[i - 1][j] > dp[i][j - 1])
        {
            --i;
        }
        else
        {
            --j;
        }
    }

    //pLfinish = std::chrono::high_resolution_clock::now();
    //pLelapsed = pLfinish - pLstart;
    //std::cout << "Retreiving LCS: " << pLelapsed.count() << " s\n";

    return pair_list;
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::LCS(Linus::jsondiff::TreeLevel level)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));

    //auto pLstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
            double score_ = _diff_level(level_, true);
            if (score_ > 0)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    //auto pLfinish = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> pLelapsed = pLfinish - pLstart;
    //std::cout << "Single thread: computing dp table: " << pLelapsed.count() << " s\n";

    //pLstart = std::chrono::high_resolution_clock::now();

    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;
    while (i > 0 && j > 0)
    {
        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
        double score_ = diff_level(level_, true);
        if (score_ > SIMILARITY_THRESHOLD)
        {
            pair_list[i - 1] = j - 1;
            --i;
            --j;
        }
        else if (dp[i - 1][j] > dp[i][j - 1])
        {
            --i;
        }
        else
        {
            --j;
        }
    }

    //pLfinish = std::chrono::high_resolution_clock::now();
    //pLelapsed = pLfinish - pLstart;
    //std::cout << "Retreive LCS: " << pLelapsed.count() << " s\n";

    return pair_list;
}

double Linus::jsondiff::JsonDiffer::compare_array_advanced(Linus::jsondiff::TreeLevel level, bool drill)
{
    std::map<unsigned int, unsigned int> pairlist;
    if (num_thread == 1)
    {
        pairlist = LCS(level);
    }
    else
    {
        pairlist = parallel_LCS(level);
    }
        
    std::vector<unsigned int> paired_left;
    std::vector<unsigned int> paired_right;
    for (const auto& pair : pairlist)
    {
        paired_left.push_back(pair.first);
        paired_right.push_back(pair.second);
    }
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    double total_score = 0;
    double score_;
    for (const auto& pair : pairlist)
    {
        Linus::jsondiff::TreeLevel level_(level.left[pair.first], level.right[pair.second], level.left_path + "[" + std::to_string(pair.first) + "]", level.right_path + "[" + std::to_string(pair.second) + "]", level.left_path);
        score_ = diff_level(level_, drill);
        total_score += score_;
    }
    rapidjson::Value emptyValue("");
    const rapidjson::Value& emptyRef = emptyValue;
    for (unsigned int index = 0; index < len_left; ++index)
    {
        if (std::find(paired_left.begin(), paired_left.end(), index) != paired_left.end())
        {
            continue;
        }
        if (!drill) 
        {
            Linus::jsondiff::TreeLevel level_(level.left[index], emptyRef, level.left_path + "[" + std::to_string(index) + "]", "", level.left_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_REMOVE, level_);
        }
    }
    for (unsigned int index = 0; index < len_right; ++index)
    {
        if (std::find(paired_right.begin(), paired_right.end(), index) != paired_right.end())
        {
            continue;
        }
        if (!drill) 
        {
            Linus::jsondiff::TreeLevel level_(emptyRef, level.right[index], "", level.right_path + "[" + std::to_string(index) + "]", level.right_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_ADD, level_);
        }
    }
    return total_score / std::max(len_left, len_right);
}

double Linus::jsondiff::JsonDiffer::compare_object(Linus::jsondiff::TreeLevel level, bool drill)
{
    double score = 0;
    std::vector<std::string> left_keys = KeysFromObject(level.left);
    std::vector<std::string> right_keys = KeysFromObject(level.right);
    std::vector<std::string> all_keys;
    all_keys.insert(all_keys.end(), left_keys.begin(), left_keys.end());
    all_keys.insert(all_keys.end(), right_keys.begin(), right_keys.end());
    std::sort(all_keys.begin(), all_keys.end());
    auto last = std::unique(all_keys.begin(), all_keys.end());
    all_keys.erase(last, all_keys.end());

    double score_;
    for (const auto& item : all_keys)
    {
        if ((std::find(left_keys.begin(), left_keys.end(), item) != left_keys.end()) && (std::find(right_keys.begin(), right_keys.end(), item) != right_keys.end()))
        {
            Linus::jsondiff::TreeLevel level_(level.left[item.c_str()], level.right[item.c_str()], level.left_path + "[\"" + item + "\"]", level.right_path + "[\"" + item + "\"]", level.left_path);
            score_ = diff_level(level_, drill);
            score += score_;
            continue;
        }

        rapidjson::Value emptyValue("");
        const rapidjson::Value& emptyRef = emptyValue;
        if (std::find(left_keys.begin(), left_keys.end(), item) != left_keys.end())
        {
            Linus::jsondiff::TreeLevel level_(level.left[item.c_str()], emptyRef, level.left_path + "[\"" + item + "\"]", "", level.left_path);
            if (!drill) 
            {
                Linus::jsondiff::JsonDiffer::report(EVENT_OBJECT_REMOVE, level_);
            }
            continue;
        }
        if (std::find(right_keys.begin(), right_keys.end(), item) != right_keys.end())
        {
            Linus::jsondiff::TreeLevel level_(emptyRef, level.right[item.c_str()], "", level.right_path + "[\"" + item + "\"]", level.right_path);
            if (!drill) 
            {
                Linus::jsondiff::JsonDiffer::report(EVENT_OBJECT_ADD, level_);
            }
            continue;
        }
    }
    if (all_keys.empty())
    {
        return 1;
    }
    return score / all_keys.size();
}

double Linus::jsondiff::JsonDiffer::compare_primitive(Linus::jsondiff::TreeLevel level, bool drill)
{
    if (level.left != level.right)
    {
        if (!drill)
        {
            Linus::jsondiff::JsonDiffer::report(EVENT_VALUE_CHANGE, level);
        }
        return 0;
    }
    return 1;
}

double Linus::jsondiff::JsonDiffer::_diff_level(Linus::jsondiff::TreeLevel level, bool drill)
{
    std::string type = level.get_type();
    if (type == "TypeMismatch")
    {
        return Linus::jsondiff::JsonDiffer::compare_primitive(level, drill);
    }
    else if (type == "Object")
    {
        return Linus::jsondiff::JsonDiffer::compare_object(level, drill);
    }
    else if (type == "Array")
    {
        return Linus::jsondiff::JsonDiffer::compare_array(level, drill);
    }
    else
    {
        return Linus::jsondiff::JsonDiffer::compare_primitive(level, drill);
    }
}

double Linus::jsondiff::JsonDiffer::diff_level(Linus::jsondiff::TreeLevel level, bool drill)
{
    std::ostringstream oss;
    oss << "[" << level.get_key() << "]@[" << std::boolalpha << drill << "]";
    std::string cache_key = oss.str();
    if (cache.find(cache_key) == cache.end())
    {
        double score = _diff_level(level, drill);
        cache[cache_key] = score;
    }
    double score = cache[cache_key];
    return score;
}

bool Linus::jsondiff::JsonDiffer::diff()
{
    Linus::jsondiff::TreeLevel root_level(left, right, "", "", "");
    return Linus::jsondiff::JsonDiffer::diff_level(root_level, false) == 1.0;
}


