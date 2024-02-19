#include "document.h"
using namespace std;
using namespace Linus::jsondiff;
using namespace rapidjson;

const std::string Linus::jsondiff::TreeLevel::empty_string = "";

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

Linus::jsondiff::TreeLevel::TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input, const std::string& path_left, const std::string& path_right, const std::string& up_level) : left(left_input), right(right_input), left_path(path_left), right_path(path_right), up(up_level)
{
    
}

Linus::jsondiff::TreeLevel::TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input) : left(left_input), right(right_input), left_path(empty_string), right_path(empty_string), up(empty_string)
{

}

int Linus::jsondiff::TreeLevel::get_type()
{
    if (left.IsObject() && right.IsObject())
    {
        return 0;
    }
    else if (left.IsArray() && right.IsArray())
    {
        return 1;
    }
    else if (left.IsString() && right.IsString())
    {
        return 2;
    }
    else if (left.IsInt() && right.IsInt())
    {
        return 3;
    }
    else if (left.IsDouble() && right.IsDouble())
    {
        return 4;
    }
    else if (left.IsBool() && right.IsBool())
    {
        return 5;
    }
    else if (left.IsNull() && right.IsNull())
    {
        return 6;
    }
    else
    {
        return 7;
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

Linus::jsondiff::JsonDiffer::JsonDiffer(const rapidjson::Value& left_input, const rapidjson::Value& right_input, bool advanced, bool hirscheburg, double similarity_threshold, int thread_count) :left(left_input), right(right_input), advanced_mode(advanced), hirscheburg(hirscheburg), SIMILARITY_THRESHOLD(similarity_threshold), num_thread(thread_count)
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
        std::ostringstream _left_path;
        _left_path << level.left_path << "[" << std::to_string(index) << "]";
        std::string left_path = _left_path.str();
        std::ostringstream _right_path;
        _right_path << level.right_path << "[" << std::to_string(index) << "]";
        std::string right_path = _right_path.str();
        Linus::jsondiff::TreeLevel level_(level.left[index], level.right[index], left_path, right_path, level.left_path);
        score_ = diff_level(level_, drill);
        total_score += score_;
    }

    rapidjson::Value emptyValue("");
    const rapidjson::Value& emptyRef = emptyValue;
    for (unsigned int index = min_len; index < len_left; ++index)
    {
        if (!drill) 
        {
            std::ostringstream _left_path;
            _left_path << level.left_path << "[" << std::to_string(index) << "]";
            std::string left_path = _left_path.str();
            Linus::jsondiff::TreeLevel level_(level.left[index], emptyRef, left_path, level.empty_string, level.left_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_REMOVE, level_);
        }
    }
    for (unsigned int index = min_len; index < len_right; ++index)
    {
        if (!drill) 
        {
            std::ostringstream _right_path;
            _right_path << level.right_path << "[" << std::to_string(index) << "]";
            std::string right_path = _right_path.str();
            Linus::jsondiff::TreeLevel level_(emptyRef, level.right[index], level.empty_string, right_path, level.right_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_ADD, level_);
        }
    }
    return total_score / max_len;
}

int Linus::jsondiff::JsonDiffer::get_type(const rapidjson::Value& input)
{
    if (input.IsObject())
    {
        return 0;
    }
    else if (input.IsArray())
    {
        return 1;
    }
    else if (input.IsString())
    {
        return 2;
    }
    else if (input.IsInt())
    {
        return 3;
    }
    else if (input.IsDouble())
    {
        return 4;
    }
    else if (input.IsBool())
    {
        return 5;
    }
    else
    {
        return 6;
    }
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
            score_[i] = (level.left[task_list[i].first].GetInt() == level.right[task_list[i].second].GetInt());
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
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));
    std::queue<std::pair<unsigned int, unsigned int>> work_queue;
    

    auto pLstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < len_left; ++i)
    {
        for (unsigned int j = 0; j < len_right; ++j)
        {
            work_queue.push(std::make_pair(i, j));
        }
    }

    auto pLfinish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> pLelapsed = pLfinish - pLstart;
    std::cout << "Building worklist: " << pLelapsed.count() << " s\n";

    std::mutex work_queue_mutex;
    std::mutex dp_mutex;
    std::vector<std::thread> threads;

    pLstart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_thread; ++i)
    {
        threads.push_back(std::thread(&Linus::jsondiff::JsonDiffer::parallel_diff_level, this, std::ref(work_queue), std::ref(dp), std::ref(level), std::ref(work_queue_mutex), std::ref(dp_mutex)));
    }

    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Allocating threads: " << pLelapsed.count() << " s\n";

    pLstart = std::chrono::high_resolution_clock::now();

    for (auto& thread : threads)
    {
        thread.join();
    }

    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Parallel computing scores: " << pLelapsed.count() << " s\n";

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
    
    pLstart = std::chrono::high_resolution_clock::now();

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
    
    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Reconstructing of dp table: " << pLelapsed.count() << " s\n";

    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;

    pLstart = std::chrono::high_resolution_clock::now();

    while (i > 0 && j > 0)
    {
        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
        double score_ = _diff_level(level_, true);
        if (score_ >= SIMILARITY_THRESHOLD)
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

    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Retreiving LCS: " << pLelapsed.count() << " s\n";

    return pair_list;
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::LCS(Linus::jsondiff::TreeLevel level, bool drill)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));

    //auto Lstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  Linus::jsondiff::JsonDiffer::get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = Linus::jsondiff::JsonDiffer::get_type(level.right[j]);
    }

    //auto Lfinish = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> getting_types = Lfinish - Lstart;
    //if(!drill)    std::cout << "LCS: getting types: " << getting_types.count() << std::endl;

    //Lstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            double score_;
            if (type_left[i-1] == type_right[j-1])
            {
                switch (type_left[i-1])
                {
                    case 0:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_obj(level.left[i-1], level.right[j-1]);
                        break;
                    }
                    case 1:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_LCS(level.left[i-1], level.right[j-1]);
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(level.left[i-1].GetString(), level.right[j-1].GetString()) == 0) ? 1 : 0;
                        break;
                    case 3:
                        score_ = (level.left[i-1].GetInt() == level.right[j-1].GetInt());
                        break;
                    case 4:
                        score_ = (level.left[i-1].GetDouble() == level.right[j-1].GetDouble());
                        break;
                    case 5:
                        score_ = (level.left[i-1].GetBool() == level.right[j-1].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            if (score_ >= SIMILARITY_THRESHOLD)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    //Lfinish = std::chrono::high_resolution_clock::now();
    //getting_types = Lfinish - Lstart;
    //if(!drill)    std::cout << "LCS: computing dp table: " << getting_types.count() << std::endl;

    //Lstart = std::chrono::high_resolution_clock::now();
    //std::cout << "Tace back" << std::endl;
    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;
    while (i > 0 && j > 0)
    {
        double score_;
        if (type_left[i-1] == type_right[j-1])
        {
            switch (type_left[i-1])
            {
                case 0:
                {
                    score_ = Linus::jsondiff::JsonDiffer::drill_obj(level.left[i-1], level.right[j-1]);
                    break;
                }
                case 1:
                {
                    score_ = Linus::jsondiff::JsonDiffer::drill_LCS(level.left[i-1], level.right[j-1]);
                    break;
                }
                case 2:
                    score_ = (std::strcmp(level.left[i - 1].GetString(), level.right[j - 1].GetString()) == 0) ? 1 : 0;
                    break;
                case 3:
                    score_ = (level.left[i - 1].GetInt() == level.right[j - 1].GetInt());
                    break;
                case 4:
                    score_ = (level.left[i - 1].GetDouble() == level.right[j - 1].GetDouble());
                    break;
                case 5:
                    score_ = (level.left[i - 1].GetBool() == level.right[j - 1].GetBool());
                    break;
                case 6:
                    score_ = 1;
                    break;
                default:
                    score_ = 0;
                    break;
            }
        }
        else
        {
            score_ = 0;
        }
        if (score_ >= SIMILARITY_THRESHOLD)
        //if ((dp[i][j] - dp[i-1][j-1]) >= SIMILARITY_THRESHOLD && (dp[i][j] - dp[i-1][j-1]) < 1)
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

    //Lfinish = std::chrono::high_resolution_clock::now();
    //getting_types = Lfinish - Lstart;
    //if(!drill)    std::cout << "LCS: retreiving dp table: " << getting_types.count() << std::endl;

    return pair_list;
}

double Linus::jsondiff::JsonDiffer::drill_LCS(const rapidjson::Value& left, const rapidjson::Value& right)
{
    unsigned int len_left = left.Size();
    unsigned int len_right = right.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    if (len_left == 0 && len_right == 0) return 1.0;
    if (len_left == 0 || len_right == 0) return 0.0;
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));

    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  Linus::jsondiff::JsonDiffer::get_type(left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = Linus::jsondiff::JsonDiffer::get_type(right[j]);
    }
    
    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            double score_;
            if (type_left[i-1] == type_right[j-1])
            {
                switch (type_left[i-1])
                {
                    case 0:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_obj(left[i-1], right[j-1]);
                        break;
                    }
                    case 1:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_LCS(left[i-1], right[j-1]);
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(left[i-1].GetString(), right[j-1].GetString()) == 0) ? 1 : 0;
                        break;
                    case 3:
                        score_ = (left[i-1].GetInt() == right[j-1].GetInt());
                        break;
                    case 4:
                        score_ = (left[i-1].GetDouble() == right[j-1].GetDouble());
                        break;
                    case 5:
                        score_ = (left[i-1].GetBool() == right[j-1].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            if (score_ >= SIMILARITY_THRESHOLD)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    return dp[len_left][len_right] / max(len_left, len_right);
}

double Linus::jsondiff::JsonDiffer::drill_obj(const rapidjson::Value& left, const rapidjson::Value& right)
{
    if (left.ObjectEmpty() && right.ObjectEmpty()) return 1.0;
    if (left.ObjectEmpty() || right.ObjectEmpty()) return 0.0;
    double score = 0;
    unsigned int count = 0;
    for (auto iter = left.MemberBegin(); iter != left.MemberEnd(); ++iter)
    {
        const char* key = iter->name.GetString();
        if (right.HasMember(key))
        {
            ++count;
            double score_;
            int type_left = Linus::jsondiff::JsonDiffer::get_type(iter->value);
            int type_right = Linus::jsondiff::JsonDiffer::get_type(right[key]);
            if (type_left == type_right)
            {
                switch (type_left)
                {
                    case 0:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_obj(iter->value, right[key]);
                        break;
                    }
                    case 1:
                    {
                        score_ = Linus::jsondiff::JsonDiffer::drill_LCS(iter->value, right[key]);
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(iter->value.GetString(), right[key].GetString()) == 0) ? 1 : 0;
                        break;
                    case 3:
                        score_ = (iter->value.GetInt() == right[key].GetInt());
                        break;
                    case 4:
                        score_ = (iter->value.GetDouble() == right[key].GetDouble());
                        break;
                    case 5:
                        score_ = (iter->value.GetBool() == right[key].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            score += score_;
        }
    }
    return score / (left.MemberCount() + right.MemberCount() - count);
}

std::vector<double> Linus::jsondiff::JsonDiffer::NWScore(bool reverse, Linus::jsondiff::TreeLevel level, bool drill, std::vector<int>& type_left, unsigned int sleft, unsigned int eleft, std::vector<int>& type_right, unsigned int sright, unsigned int eright)
{
    //if (reverse) std::cout << "Reverse ";
    //std::cout << "NWScore sleft " << sleft << " eleft " << eleft << " sright " << sright << " eright " << eright << std::endl;
    unsigned int len_left = eleft - sleft;
    unsigned int len_right = eright - sright;
    std::vector<std::vector<double>> dp(2, std::vector<double>(len_right + 2, 0));
    dp[0][0] = 0;
    for (int j = 1; j <= len_right; ++j)
    {
        dp[0][j] = dp[0][j - 1];
    }
    if (reverse)
    {
        for (int i = 1; i <= len_left; ++i)
        {
            dp[1][0] = dp[0][0];
            for (int j = 1; j <= len_right; ++j)
            {
                //std::cout << "left index " << eleft-i << " right " << eright-j << std::endl;
                double score_;
                if (type_left[eleft - i] == type_right[eright - j])
                {
                    switch (type_left[eleft - i])
                    {
                        case 0:
                        {
                            //Linus::jsondiff::TreeLevel level_(level.left[eleft - i], level.right[eright - j]);
                            //score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                            score_ = Linus::jsondiff::JsonDiffer::drill_obj(level.left[eleft-i], level.right[eright-j]);
                            break;
                        }
                        case 1:
                        {
                            //Linus::jsondiff::TreeLevel level_(level.left[eleft - i], level.right[eright - j]);
                            //score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                            score_ = Linus::jsondiff::JsonDiffer::drill_LCS(level.left[eleft-i], level.right[eright-j]);
                            break;
                        }
                        case 2:
                            score_ = (std::strcmp(level.left[eleft-i].GetString(), level.right[eright-j].GetString()));
                            break;
                        case 3:
                            score_ = (level.left[eleft-i].GetInt() == level.right[eright-j].GetInt());
                            break;
                        case 4:
                            score_ = (level.left[eleft-i].GetDouble() == level.right[eright-j].GetDouble());
                            break;
                        case 5:
                            score_ = (level.left[eleft-i].GetBool() == level.right[eright-j].GetBool());
                            break;
                        case 6:
                            score_ = 1;
                            break;
                        default:
                            score_ = 0;
                            break;
                    }
                }
                else
                {
                    score_ = 0;
                }
                dp[1][j] = std::max(dp[0][j - 1] + score_, std::max(dp[0][j], dp[1][j - 1]));
            }
            dp[0] = dp[1];
        }
    }
    else
    {
        for (int i = 1; i <= len_left; ++i)
        {
            dp[1][0] = dp[0][0];
            for (int j = 1; j <= len_right; ++j)
            {
                double score_;
                if (type_left[sleft + i - 1] == type_right[sright + j - 1])
                {
                    switch (type_left[sleft + i - 1])
                    {
                        case 0:
                        {
                            //Linus::jsondiff::TreeLevel level_(level.left[sleft + i - 1], level.right[sright + j - 1]);
                            //Linus::jsondiff::TreeLevel level_(level.left[1], level.right[23]);
                            //score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                            score_ = Linus::jsondiff::JsonDiffer::drill_obj(level.left[sleft+i-1], level.right[sright+j-1]);
                            break;
                        }
                        case 1:
                        {
                            //Linus::jsondiff::TreeLevel level_(level.left[sleft + i - 1], level.right[sright + j - 1]);
                            //score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                            score_ = Linus::jsondiff::JsonDiffer::drill_LCS(level.left[sleft+i-1], level.right[sright+j-1]);
                            break;
                        }
                        case 2:
                            score_ = (std::strcmp(level.left[sleft+i-1].GetString(), level.right[sright+j-1].GetString()) == 0);
                            break;
                        case 3:
                            score_ = (level.left[sleft+i-1].GetInt() == level.right[sright+j-1].GetInt());
                            break;
                        case 4:
                            score_ = (level.left[sleft+i-1].GetDouble() == level.right[sright+j-1].GetDouble());
                            break;
                        case 5:
                            score_ = (level.left[sleft+i-1].GetBool() == level.right[sright+j-1].GetBool());
                            break;
                        case 6:
                            score_ = 1;
                            break;
                        default:
                            score_ = 0;
                            break;
                    }
                }
                else
                {
                    score_ = 0;
                }
                dp[1][j] = std::max(dp[0][j - 1] + score_, std::max(dp[0][j], dp[1][j - 1]));
            }
            dp[0] = dp[1];
        }
    }
    return dp[1];
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::Hirschberg(Linus::jsondiff::TreeLevel level, bool drill, std::vector<int>& type_left, unsigned int sleft, unsigned int eleft, std::vector<int>& type_right, unsigned int sright, unsigned int eright)
{
    //std::cout << "Hirschberg sleft " << sleft << " eleft " << eleft << " sright " << sright << " eright " << eright << std::endl;
    unsigned int len_left = eleft - sleft + 1;
    //std::cout << "len_left " << len_left << std::endl;
    unsigned int len_right = eright - sright + 1;
    std::map<unsigned int, unsigned int> pair_list;
    /*directly report here*/
    if (len_left == 0 || len_right == 0)
    {
        return pair_list;
    }
    if (len_left == 1 || len_right == 1)/////////////////////////////////////////////////////////
    {
        double score_;
        if (type_left[sleft] == type_right[sright])
        {
            switch (type_left[sleft])
            {
                case 0:
                {
                    //Linus::jsondiff::TreeLevel level_(level.left[sleft], level.right[sright]);
                    //score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                    score_ = Linus::jsondiff::JsonDiffer::drill_obj(level.left[sleft], level.right[sright]);
                    break;
                }
                case 1:
                {
                    //Linus::jsondiff::TreeLevel level_(level.left[sleft], level.right[sright]);
                    //score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                    score_ = Linus::jsondiff::JsonDiffer::drill_LCS(level.left[sleft], level.right[sright]);
                    break;
                }
                case 2:
                    score_ = (std::strcmp(level.left[sleft].GetString(), level.right[sright].GetString()) == 0);
                    break;
                case 3:
                    score_ = (level.left[sleft].GetInt() == level.right[sright].GetInt());
                    break;
                case 4:
                    score_ = (level.left[sleft].GetDouble() == level.right[sright].GetDouble());
                    break;
                case 5:
                    score_ = (level.left[sleft].GetBool() == level.right[sright].GetBool());
                    break;
                case 6:
                    score_ = 1;
                    break;
                default:
                    score_ = 0;
                    break;
            }
        }
        else
        {
            score_ = 0;
        }
        if (score_ >= SIMILARITY_THRESHOLD)
        {
            pair_list[sleft] = sright;
        }
    }
    else
    {
        unsigned int left_mid = (sleft + eleft) / 2;
        //std::cout << "left_mid " << left_mid << std::endl;
        std::vector<double> scoreL = NWScore(false, level, true, type_left, sleft, left_mid, type_right, sright, eright);
        std::vector<double> scoreR = NWScore(true, level, true, type_left, left_mid + 1, eleft, type_right, sright, eright);
        std::reverse(scoreR.begin(), scoreR.end());
        unsigned int right_mid;
        double max = -1;
        for (unsigned int i = 0; i < scoreL.size(); ++i)
        {
            double score_ = std::ceil(scoreL[i] + scoreR[i]);
            if (score_ > max)
            {
                max = score_;
                right_mid = sright + i;
            }
        }
        //std::cout << "right_mid " << right_mid << std::endl;
        for (const auto& pair : Hirschberg(level, drill, type_left, sleft, left_mid, type_right, sright, right_mid)) 
        {
            pair_list.insert(pair);
        }
        for (const auto& pair : Hirschberg(level, drill, type_left, left_mid + 1, eleft, type_right, right_mid + 1, eright)) 
        {
            pair_list.insert(pair);
        }
    }
    return pair_list;
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::Hirschberg_starter(Linus::jsondiff::TreeLevel level)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  Linus::jsondiff::JsonDiffer::get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = Linus::jsondiff::JsonDiffer::get_type(level.right[j]);
    }
    return Hirschberg(level, true, type_left, 0, len_left-1, type_right, 0, len_right-1);
}

double Linus::jsondiff::JsonDiffer::compare_array_advanced(Linus::jsondiff::TreeLevel level, bool drill)
{
    std::map<unsigned int, unsigned int> pairlist;
    //auto start = std::chrono::high_resolution_clock::now();
    if (num_thread == 1)
    {
        if(!hirscheburg) pairlist = LCS(level, drill);
        else pairlist = Hirschberg_starter(level);
        //pairlist = Hirschberg_starter(level);
        /*Linus::jsondiff::BottomUpLCS BU(level, *this);
        BU.bu_computing();
        pairlist = BU.LCS();*/
    }
    else
    {
        pairlist = parallel_LCS(level);
    }
    //auto finish = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> elapsed = finish - start;
    //std::cout << "TopDown LCS: " << elapsed.count() << " s\n";
    //std::cout << "BottomUp time: " << elapsed.count() << " s\n";
    std::vector<unsigned int> paired_left;
    std::vector<unsigned int> paired_right;
    for (const auto& pair : pairlist)
    {
        paired_left.push_back(pair.first);
        paired_right.push_back(pair.second);
        //std::cout << "Pair Left " << pair.first << " Right " << pair.second << std::endl;
    }
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    double total_score = 0;
    double score_;
    for (const auto& pair : pairlist)
    {
        if (!drill)
        {
            std::ostringstream _left_path;
            _left_path << level.left_path << "[" << std::to_string(pair.first) << "]";
            std::string left_path = _left_path.str();
            std::ostringstream _right_path;
            _right_path << level.right_path << "[" << std::to_string(pair.second) << "]";
            std::string right_path = _right_path.str();
            Linus::jsondiff::TreeLevel level_(level.left[pair.first], level.right[pair.second], left_path, right_path, level.left_path);
            score_ = diff_level(level_, drill);
            total_score += score_;
            //std::cout << "Pair Left " << pair.first << " Right " << pair.second << std::endl;
        }
        else
        {
            Linus::jsondiff::TreeLevel level_(level.left[pair.first], level.right[pair.second]);
            score_ = _diff_level(level_, drill);
        }
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
            std::ostringstream _left_path;
            _left_path << level.left_path << "[" << std::to_string(index) << "]";
            std::string left_path = _left_path.str();
            Linus::jsondiff::TreeLevel level_(level.left[index], emptyRef, left_path, level.empty_string, level.left_path);
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
            std::ostringstream _right_path;
            _right_path << level.right_path << "[" << std::to_string(index) << "]";
            std::string right_path = _right_path.str();
            Linus::jsondiff::TreeLevel level_(emptyRef, level.right[index], level.empty_string, right_path, level.right_path);
            Linus::jsondiff::JsonDiffer::report(EVENT_ARRAY_ADD, level_);
        }
    }
    //std::cout << "Compare_array_advanced finished" << std::endl;
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
            if (!drill)
            {
                std::ostringstream _left_path;
                _left_path << level.left_path << "[\"" << item << "\"]";
                std::string left_path = _left_path.str();
                std::ostringstream _right_path;
                _right_path << level.right_path << "[\"" << item << "\"]";
                std::string right_path = _right_path.str();
                Linus::jsondiff::TreeLevel level_(level.left[item.c_str()], level.right[item.c_str()], left_path, right_path, level.left_path);
                score_ = diff_level(level_, drill);
            }
            else
            {
                Linus::jsondiff::TreeLevel level_(level.left[item.c_str()], level.right[item.c_str()]);
                score_ = _diff_level(level_, drill);
            }
            score += score_;
            continue;
        }

        rapidjson::Value emptyValue("");
        const rapidjson::Value& emptyRef = emptyValue;
        if (std::find(left_keys.begin(), left_keys.end(), item) != left_keys.end())
        {
            if (!drill) 
            {
                std::ostringstream _left_path;
                _left_path << level.left_path << "[\"" << item << "\"]";
                std::string left_path = _left_path.str();
                Linus::jsondiff::TreeLevel level_(level.left[item.c_str()], emptyRef, left_path, level.empty_string, level.left_path);
                Linus::jsondiff::JsonDiffer::report(EVENT_OBJECT_REMOVE, level_);
            }
            continue;
        }
        if (std::find(right_keys.begin(), right_keys.end(), item) != right_keys.end())
        {
            if (!drill) 
            {
                std::ostringstream _right_path;
                _right_path << level.right_path << "[\"" << item << "\"]";
                std::string right_path = _right_path.str();
                Linus::jsondiff::TreeLevel level_(emptyRef, level.right[item.c_str()], level.empty_string, right_path, level.right_path);
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

double Linus::jsondiff::JsonDiffer::compare_Int(Linus::jsondiff::TreeLevel level, bool drill)
{
    if (level.left.GetInt() != level.right.GetInt())
    {
        if (!drill)
        {
            Linus::jsondiff::JsonDiffer::report(EVENT_VALUE_CHANGE, level);
        }
        return 0;
    }
    return 1;
}

double Linus::jsondiff::JsonDiffer::compare_Double(Linus::jsondiff::TreeLevel level, bool drill)
{
    if (level.left.GetDouble() != level.right.GetDouble())
    {
        if (!drill)
        {
            Linus::jsondiff::JsonDiffer::report(EVENT_VALUE_CHANGE, level);
        }
        return 0;
    }
    return 1;
}

double Linus::jsondiff::JsonDiffer::compare_String(Linus::jsondiff::TreeLevel level, bool drill)
{
    if (std::strcmp(level.left.GetString(), level.right.GetString()))
    {
        if (!drill)
        {
            Linus::jsondiff::JsonDiffer::report(EVENT_VALUE_CHANGE, level);
        }
        return 0;
    }
    return 1;
}

double Linus::jsondiff::JsonDiffer::compare_Bool(Linus::jsondiff::TreeLevel level, bool drill)
{
    if (level.left.GetBool() != level.right.GetBool())
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
    int type = level.get_type();
    switch (type)
    {
    case 0:
        return Linus::jsondiff::JsonDiffer::compare_object(level, drill);
        break;
    case 1:
        return Linus::jsondiff::JsonDiffer::compare_array(level, drill);
        break;
    case 2:
        return Linus::jsondiff::JsonDiffer::compare_String(level, drill);
        break;
    case 3:
        return Linus::jsondiff::JsonDiffer::compare_Int(level, drill);
        break;
    case 4:
        return Linus::jsondiff::JsonDiffer::compare_Double(level, drill);
        break;
    case 5:
        return Linus::jsondiff::JsonDiffer::compare_Bool(level, drill);
        break;
    case 6:
        return 1;
        break;
    case 7:
        if (!drill)
        {
            Linus::jsondiff::JsonDiffer::report(EVENT_VALUE_CHANGE, level);
        }
        return 0;
        break;
    default:
        return 0;
        break;
    }
}

double Linus::jsondiff::JsonDiffer::diff_level(Linus::jsondiff::TreeLevel level, bool drill)
{
    /*std::ostringstream oss;
    oss << "[" << level.get_key() << "]@[" << std::boolalpha << drill << "]";
    std::string cache_key = oss.str();
    if (cache.find(cache_key) == cache.end())
    {
        double score = _diff_level(level, drill);
        cache[cache_key] = score;
    }
    double score = cache[cache_key];
    return score;*/
    return _diff_level(level, drill);
}

bool Linus::jsondiff::JsonDiffer::diff()
{
    Linus::jsondiff::TreeLevel root_level(left, right);
    return Linus::jsondiff::JsonDiffer::diff_level(root_level, false) == 1.0;
}

Linus::jsondiff::BottomUpLCS::BottomUpLCS(Linus::jsondiff::TreeLevel& level, Linus::jsondiff::JsonDiffer& differ) : level(level), differ(differ)
{
    Linus::jsondiff::BottomUpLCS::locate_left_array(level.left, 0);
    Linus::jsondiff::BottomUpLCS::locate_right_array(level.right, 0);
}

void Linus::jsondiff::BottomUpLCS::locate_left_array(const rapidjson::Value& tree, unsigned int layer)
{
    if (tree.IsArray())
    {
        if(layer != 0) left_array[layer].push_back(&tree);
        unsigned int len = tree.Size();
        for (unsigned int i = 0; i < len; ++i)
        {
            locate_left_array(tree[i], layer + 1);
        }
    }
    else if (tree.IsObject())
    {
        for (rapidjson::Value::ConstMemberIterator itr = tree.MemberBegin(); itr != tree.MemberEnd(); ++itr)
        {
            locate_left_array(tree[itr->name], layer + 1);
        }
    }
    else
    {
        return;
    }
}

void Linus::jsondiff::BottomUpLCS::locate_right_array(const rapidjson::Value& tree, unsigned int layer)
{
    if (tree.IsArray())
    {
        if(layer != 0) right_array[layer].push_back(&tree);
        unsigned int len = tree.Size();
        for (unsigned int i = 0; i < len; ++i)
        {
            locate_right_array(tree[i], layer + 1);
        }
    }
    else if (tree.IsObject())
    {
        for (rapidjson::Value::ConstMemberIterator itr = tree.MemberBegin(); itr != tree.MemberEnd(); ++itr)
        {
            locate_right_array(tree[itr->name], layer + 1);
        }
    }
    else
    {
        return;
    }
}

void Linus::jsondiff::BottomUpLCS::bu_computing()
{
    unsigned int deepest = std::min(left_array.rbegin()->first, right_array.rbegin()->first);
    unsigned int highest = std::max(left_array.begin()->first, right_array.begin()->first);
    for (unsigned int i = deepest; i >= highest; --i)
    {
        for (auto lit = left_array[i].begin(); lit != left_array[i].end(); ++lit)
        {
            for (auto rit = right_array[i].begin(); rit != right_array[i].end(); ++rit)
            {
                Linus::jsondiff::BottomUpLCS::inter_LCS(*lit, *rit, i);
            }
        }
        if (i != left_array.rbegin()->first && i != right_array.rbegin()->first)
        {
            history[i+1].clear();
        }
    }
}

void Linus::jsondiff::BottomUpLCS::inter_LCS(const rapidjson::Value* ptr_left, const rapidjson::Value* ptr_right, unsigned int layer)
{
    const rapidjson::Value& left = *ptr_left;
    const rapidjson::Value& right = *ptr_right;
    unsigned int len_left = left.Size();
    unsigned int len_right = right.Size();
    if (len_left == 0 && len_right == 0)
    {
        history[layer][ptr_left][ptr_right] = 1.0;
        return;
    }
    if (len_left == 0 || len_right == 0)
    {
        history[layer][ptr_left][ptr_right] = 0.0;
        return;
    }
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left.push_back(differ.get_type(left[i]));
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right.push_back(differ.get_type(right[j]));
    }
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));
    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            double score_;
            if (type_left[i-1] == type_right[j-1])
            {
                switch (type_left[i-1])
                {
                    case 0:
                    {
                        score_ = compare_object(left[i - 1], right[j - 1], layer + 1);
                        break;
                    }
                    case 1:
                    {
                        score_ = history[layer + 1][&left[i - 1]][&right[j - 1]];
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(left[i-1].GetString(), right[j-1].GetString()));
                        break;
                    case 3:
                        score_ = (left[i-1].GetInt() == right[j-1].GetInt());
                        break;
                    case 4:
                        score_ = (left[i-1].GetDouble() == right[j-1].GetDouble());
                        break;
                    case 5:
                        score_ = (left[i-1].GetBool() == right[j-1].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            if (score_ >= differ.SIMILARITY_THRESHOLD)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    history[layer][ptr_left][ptr_right] = dp[len_left][len_right] / max(len_left, len_right);
}

double Linus::jsondiff::BottomUpLCS::compare_object(const rapidjson::Value& left, const rapidjson::Value& right, unsigned int layer)
{
    if (left.ObjectEmpty() && right.ObjectEmpty()) return 1.0;
    if (left.ObjectEmpty() || right.ObjectEmpty()) return 0.0;
    double score = 0;
    unsigned int count = 0;
    for (auto iter = left.MemberBegin(); iter != left.MemberEnd(); ++iter)
    {
        const char* key = iter->name.GetString();
        if (right.HasMember(key))
        {
            ++count;
            double score_;
            int type_left = differ.get_type(iter->value);
            int type_right = differ.get_type(right[key]);
            if (type_left == type_right)
            {
                //std::cout << "Value Left " << iter->value.GetString() << " Right " << right[key].GetString() << std::endl;
                switch (type_left)
                {
                    case 0:
                    {
                        score_ = Linus::jsondiff::BottomUpLCS::compare_object(iter->value, right[key], layer + 1);
                        break;
                    }
                    case 1:
                    {
                        score_ = history[layer + 1][&iter->value][&right[key]];
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(iter->value.GetString(), right[key].GetString()) == 0) ? 1 : 0;
                        break;
                    case 3:
                        score_ = (iter->value.GetInt() == right[key].GetInt());
                        break;
                    case 4:
                        score_ = (iter->value.GetDouble() == right[key].GetDouble());
                        break;
                    case 5:
                        score_ = (iter->value.GetBool() == right[key].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            score += score_;
        }
    }
    return score / (left.MemberCount() + right.MemberCount() - count);
}

std::map<unsigned int, unsigned int> Linus::jsondiff::BottomUpLCS::LCS()
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));
    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  differ.get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = differ.get_type(level.right[j]);
    }
    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            double score_;
            if (type_left[i-1] == type_right[j-1])
            {
                switch (type_left[i-1])
                {
                    case 0:
                    {
                        score_ = Linus::jsondiff::BottomUpLCS::compare_object(level.left[i - 1], level.right[j - 1], 1);
                        break;
                    }
                    case 1:
                    {
                        score_ = history[1][&level.left[i - 1]][&level.right[j - 1]];
                        //std::cout << score_ << std::endl;
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(level.left[i-1].GetString(), level.right[j-1].GetString()));
                        break;
                    case 3:
                        score_ = (level.left[i-1].GetInt() == level.right[j-1].GetInt());
                        break;
                    case 4:
                        score_ = (level.left[i-1].GetDouble() == level.right[j-1].GetDouble());
                        break;
                    case 5:
                        score_ = (level.left[i-1].GetBool() == level.right[j-1].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
            }
            else
            {
                score_ = 0;
            }
            if (score_ >= differ.SIMILARITY_THRESHOLD)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;
    while (i > 0 && j > 0)
    {
        double score_;
        if (type_left[i-1] == type_right[j-1])
        {
            switch (type_left[i-1])
            {
                case 0:
                {
                    score_ = Linus::jsondiff::BottomUpLCS::compare_object(level.left[i - 1], level.right[j - 1], 1);
                    break;
                }
                case 1:
                {
                    score_ = history[1][&level.left[i - 1]][&level.right[j - 1]];
                    break;
                }
                case 2:
                    score_ = (std::strcmp(level.left[i - 1].GetString(), level.right[j - 1].GetString()));
                    break;
                case 3:
                    score_ = (level.left[i - 1].GetInt() == level.right[j - 1].GetInt());
                    break;
                case 4:
                    score_ = (level.left[i - 1].GetDouble() == level.right[j - 1].GetDouble());
                    break;
                case 5:
                    score_ = (level.left[i - 1].GetBool() == level.right[j - 1].GetBool());
                    break;
                case 6:
                    score_ = 1;
                    break;
                default:
                    score_ = 0;
                    break;
            }
        }
        else
        {
            score_ = 0;
        }
        if (score_ >= differ.SIMILARITY_THRESHOLD)
        //if (score_ == 1)
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
    history.clear();
    return pair_list;
}