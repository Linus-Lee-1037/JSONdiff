#include "document.h"
using namespace std;
using namespace Linus::jsondiff;
using namespace rapidjson;

const std::string Linus::jsondiff::TreeLevel::empty_string = "";
const std::string BottomUpLCS::empty_string = "";

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
                        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                        score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                        break;
                    }
                    case 1:
                    {
                        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                        score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(level.left[i-1].GetString(), level.right[i-1].GetString()));
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
            if (score_ > SIMILARITY_THRESHOLD)
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
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                    score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                    break;
                }
                case 1:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                    score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
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

    //Lfinish = std::chrono::high_resolution_clock::now();
    //getting_types = Lfinish - Lstart;
    //if(!drill)    std::cout << "LCS: retreiving dp table: " << getting_types.count() << std::endl;

    return pair_list;
}

std::vector<double> Linus::jsondiff::JsonDiffer::NWScore(bool reverse, Linus::jsondiff::TreeLevel level, bool drill, std::vector<int>& type_left, unsigned int sleft, unsigned int eleft, std::vector<int>& type_right, unsigned int sright, unsigned int eright)
{
    unsigned int len_left = eleft - sleft;
    unsigned int len_right = eright - sright;
    std::vector<std::vector<double>> dp(2, std::vector<double>(len_right + 1, 0));
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
                double score_;
                if (type_left[eleft - i] == type_right[eright - j])
                {
                    switch (type_left[eleft - i])
                    {
                        case 0:
                        {
                            Linus::jsondiff::TreeLevel level_(level.left[eleft - i], level.right[eright - j]);
                            score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                            break;
                        }
                        case 1:
                        {
                            Linus::jsondiff::TreeLevel level_(level.left[eleft - i], level.right[eright - j]);
                            score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                            break;
                        }
                        case 2:
                            score_ = (std::strcmp(level.left[eleft - i].GetString(), level.right[eright - j].GetString()));
                            break;
                        case 3:
                            score_ = (level.left[eleft - i].GetInt() == level.right[eright - j].GetInt());
                            break;
                        case 4:
                            score_ = (level.left[eleft - i].GetDouble() == level.right[eright - j].GetDouble());
                            break;
                        case 5:
                            score_ = (level.left[eleft - i].GetBool() == level.right[eright - j].GetBool());
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
                            Linus::jsondiff::TreeLevel level_(level.left[1], level.right[23]);
                            score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                            break;
                        }
                        case 1:
                        {
                            Linus::jsondiff::TreeLevel level_(level.left[sleft + i - 1], level.right[sright + j - 1]);
                            score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                            break;
                        }
                        case 2:
                            score_ = (std::strcmp(level.left[sleft + i - 1].GetString(), level.right[sright + j - 1].GetString()));
                            break;
                        case 3:
                            score_ = (level.left[sleft + i - 1].GetInt() == level.right[sright + j - 1].GetInt());
                            break;
                        case 4:
                            score_ = (level.left[sleft + i - 1].GetDouble() == level.right[sright + j - 1].GetDouble());
                            break;
                        case 5:
                            score_ = (level.left[sleft + i - 1].GetBool() == level.right[sright + j - 1].GetBool());
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
    //std::cout << "sleft " << sleft << " eleft " << eleft << " sright " << sright << " eright " << eright << std::endl;
    unsigned int len_left = eleft - sleft;
    //std::cout << "len_left " << len_left << std::endl;
    unsigned int len_right = eright - sright;
    std::map<unsigned int, unsigned int> pair_list;
    /*directly report here*/
    if (len_left == 1 || len_right == 1)/////////////////////////////////////////////////////////
    {
        double score_;
        if (type_left[sleft] == type_right[sright])
        {
            switch (type_left[sleft])
            {
                case 0:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[sleft], level.right[sright]);
                    score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                    break;
                }
                case 1:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[sleft], level.right[sright]);
                    score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                    break;
                }
                case 2:
                    score_ = (std::strcmp(level.left[sleft].GetString(), level.right[sright].GetString()));
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
        if (score_ > SIMILARITY_THRESHOLD)
        {
            pair_list[sleft] = sright;
        }
    }
    else
    {
        unsigned int left_mid = len_left / 2;
        //std::cout << "left_mid " << left_mid << std::endl;
        std::vector<double> scoreL = NWScore(false, level, true, type_left, sleft, left_mid, type_right, sright, eright);
        std::vector<double> scoreR = NWScore(true, level, true, type_left, left_mid, eleft, type_right, sright, eright);
        std::reverse(scoreR.begin(), scoreR.end());
        unsigned int right_mid;
        double max = -1;
        for (unsigned int i = 0; i < scoreL.size(); ++i)
        {
            double score_ = std::ceil(scoreL[i] + scoreR[i]);
            if (score_ > max)
            {
                max = score_;
                right_mid = i;
            }
        }
        for (const auto& pair : Hirschberg(level, drill, type_left, sleft, left_mid, type_right, sright, right_mid)) 
        {
            pair_list.insert(pair);
        }
        for (const auto& pair : Hirschberg(level, drill, type_left, left_mid, eleft, type_right, right_mid, eright)) 
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
    return Hirschberg(level, true, type_left, 0, level.left.Size(), type_right, 0, level.right.Size());
}

double Linus::jsondiff::JsonDiffer::compare_array_advanced(Linus::jsondiff::TreeLevel level, bool drill)
{
    std::map<unsigned int, unsigned int> pairlist;
    if (num_thread == 1)
    {
        //pairlist = LCS(level, drill);
        //pairlist = Hirschberg_starter(level);
        Linus::jsondiff::BottomUpLCS BU(level, *this);
        pairlist = BU.bu_computing();
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
    Linus::jsondiff::TreeLevel root_level(left, right);
    return Linus::jsondiff::JsonDiffer::diff_level(root_level, false) == 1.0;
}

Linus::jsondiff::BottomUpLCS::BottomUpLCS(Linus::jsondiff::TreeLevel level_, Linus::jsondiff::JsonDiffer& differ) : level(level_), jsondiffer(differ)
{
    locate_array(level.left, empty_string, 0, false);   
    locate_array(level.right, empty_string, 0, true);
}

int Linus::jsondiff::BottomUpLCS::get_type(const rapidjson::Value& input)
{
    if (input.IsObject())
    {
        return 0;
    }
    else if (input.IsArray())
    {
        return 1;
    }
    else
    {
        return 6;
    }
}

void Linus::jsondiff::BottomUpLCS::locate_array(const rapidjson::Value& tree, const std::string& path, unsigned int layer, bool lr)
{
    int tree_type = Linus::jsondiff::BottomUpLCS::get_type(tree);
    switch (tree_type)
    {
        case 0:{
            std::vector<std::string> keys = KeysFromObject(tree);
            for (const auto& item : keys)
            {
                int type = Linus::jsondiff::BottomUpLCS::get_type(tree[item.c_str()]);
                std::ostringstream new_path;
                new_path << path << "/" << item;
                switch (type)
                {
                    case 0:
                        locate_array(tree[item.c_str()], new_path.str(), layer + 1, lr);
                        break;
                    case 1:
                        if (lr)
                        {
                            right_nested_array[layer + 1].push_back(new_path.str());
                        }
                        else
                        {
                            left_nested_array[layer + 1].push_back(new_path.str());
                        }
                        locate_array(tree[item.c_str()], new_path.str(), layer + 1, lr);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case 1: {
            unsigned int length = tree.Size();
            for (unsigned int i = 0; i < length; ++i)
            {
                int type = Linus::jsondiff::BottomUpLCS::get_type(tree[i]);
                std::ostringstream new_path;
                new_path << path << "/" << std::to_string(i);
                switch (type)
                {
                    case 0:
                        locate_array(tree[i], new_path.str(), layer + 1, lr);
                        break;
                    case 1:
                        if (lr)
                        {
                            right_nested_array[layer + 1].push_back(new_path.str());
                        }
                        else
                        {
                            left_nested_array[layer + 1].push_back(new_path.str());
                        }
                        locate_array(tree[i], new_path.str(), layer + 1, lr);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        default:
            break;
    }  
}

bool Linus::jsondiff::BottomUpLCS::isUInt(const std::string& s) 
{
    if (s.empty() || (!isdigit(s[0]) && s[0] != '+')) return false;
    char* p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

bool Linus::jsondiff::BottomUpLCS::path_same(const std::string& left_path, const std::string& right_path)
{
    std::string trimmedS1 = left_path.substr(1);
    std::string trimmedS2 = right_path.substr(1);

    std::istringstream stream1(trimmedS1);
    std::istringstream stream2(trimmedS2);
    std::string segment1, segment2;

    while (std::getline(stream1, segment1, '/') && std::getline(stream2, segment2, '/')) 
    {
        bool isSegment1UInt = isUInt(segment1);
        bool isSegment2UInt = isUInt(segment2);

        if (!isSegment1UInt || !isSegment2UInt) 
        {
            if (!isSegment1UInt && !isSegment2UInt) 
            {
                if (segment1 != segment2) 
                {
                    return false;
                }
            } 
            else 
            {
                return false;
            }
        }
    }
    return true;
}

std::map<unsigned int, unsigned int> Linus::jsondiff::BottomUpLCS::bu_computing()
{
    unsigned int deepest = min(left_nested_array.rbegin()->first, right_nested_array.rbegin()->first);
    unsigned int highest = max(left_nested_array.begin()->first, right_nested_array.begin()->first);
    for (int i = deepest; i >= highest; --i)
    {
        for (const auto& left_path : left_nested_array[i])
        {
            for (const auto& right_path : right_nested_array[i])
            {
                if (path_same(left_path, right_path))
                {
                    LCS(left_path, right_path);
                }
            }
        }
        std::cout << "layer: " << i << std::endl;
    }
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));
    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            std::ostringstream query;
            query << "[/" << std::to_string(i - 1) << "]@[/" << std::to_string(j - 1) << "]";
            dp[i][j] = history[query.str()];
        }
    }
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  jsondiffer.get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = jsondiffer.get_type(level.right[j]);
    }
    std::map<unsigned int, unsigned int> pair_list;
    unsigned int i = len_left;
    unsigned int j = len_right;
    while (i > 0 && j > 0)
    {
        double score_ = 0;
        if (type_left[i - 1] == type_right[j - 1])
        {
            switch (type_left[i - 1])
            {
                case 0:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                    score_ = jsondiffer.compare_object(level_, true);
                    break;
                }
                case 1:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1]);
                    score_ = jsondiffer.compare_array(level_, true);
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
        if (score_ > jsondiffer.SIMILARITY_THRESHOLD)
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
    return pair_list;
}

void Linus::jsondiff::BottomUpLCS::LCS(const std::string& left_path, const std::string& right_path)
{
    double score = 0;
    const rapidjson::Value& left_value = *rapidjson::Pointer(left_path.c_str()).Get(level.left);
    const rapidjson::Value& right_value = *rapidjson::Pointer(right_path.c_str()).Get(level.right);
    unsigned int len_left = left_value.Size();
    unsigned int len_right = right_value.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));

    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  jsondiffer.get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = jsondiffer.get_type(level.right[j]);
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
                        Linus::jsondiff::TreeLevel level_(left_value[i - 1], right_value[j - 1]);
                        std::ostringstream path_left;
                        path_left << left_path << "/" << std::to_string(i - 1);
                        std::ostringstream path_right;
                        path_right << right_path << "/" << std::to_string(j - 1);
                        score_ = object_compare(level_, path_left.str(), path_right.str());
                        break;
                    }
                    case 1:
                    {
                        std::ostringstream query;
                        query << "[" << left_path << "/" << std::to_string(i - 1) << "]@[" << right_path << "/" << std::to_string(i - 1) << "]";
                        score_ = history[query.str()];
                        break;
                    }
                    case 2:
                        score_ = (std::strcmp(left_value[i - 1].GetString(), right_value[j - 1].GetString()));
                        break;
                    case 3:
                        score_ = (left_value[i-1].GetInt() == right_value[j-1].GetInt());
                        break;
                    case 4:
                        score_ = (left_value[i-1].GetDouble() == right_value[j-1].GetDouble());
                        break;
                    case 5:
                        score_ = (left_value[i-1].GetBool() == right_value[j-1].GetBool());
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
            if (score_ > jsondiffer.SIMILARITY_THRESHOLD)
            {
                dp[i][j] = dp[i - 1][j - 1] + score_;
            }
            else
            {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    std::ostringstream query;
    query << "[" << left_path << "]@[" << right_path << "]";
    history[query.str()] = dp[len_left][len_right];
}

double Linus::jsondiff::BottomUpLCS::object_compare(Linus::jsondiff::TreeLevel level_, const std::string& left_path, const std::string& right_path)
{
    double score = 0;
    std::vector<std::string> left_keys = KeysFromObject(level_.left);
    std::vector<std::string> right_keys = KeysFromObject(level_.right);
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
            int left_type = jsondiffer.get_type(level_.left[item.c_str()]);
            int right_type = jsondiffer.get_type(level_.right[item.c_str()]);
            if (left_type == right_type)
            {
                switch (left_type)
                {
                    case 0:
                    {
                        Linus::jsondiff::TreeLevel _level(level_.left[item.c_str()], level_.right[item.c_str()]);
                        std::ostringstream path_left;
                        path_left << left_path << "/" << item;
                        std::ostringstream path_right;
                        path_right << right_path << "/" << item;
                        //std::cout << path_left.str() << " " << path_right.str() << std::endl;
                        score_ = object_compare(_level, path_left.str(), path_right.str());
                        break;
                    }
                    case 1:
                    {
                        std::ostringstream query;
                        query << "[" << left_path << "/" << item << "]@[" << right_path << "/" << item << "]";
                        score_ = history[query.str()];
                        break;
                    }
                    case 2:
                    {
                        /*if (left_path == "/9487/payload/release/assets/2/uploader" && right_path == "/9487/payload/release/assets/2/uploader")
                        {
                            std::ostringstream path_left;
                            path_left << left_path << "/" << item;
                            std::ostringstream path_right;
                            path_right << right_path << "/" << item;
                            std::cout << path_left.str() << " " << path_right.str() << std::endl;
                        }*/
                        score_ = (std::strcmp(level_.left[item.c_str()].GetString(), level_.right[item.c_str()].GetString()));
                        break;
                    }
                    case 3:
                        score_ = (level_.left[item.c_str()].GetInt() == level_.right[item.c_str()].GetInt());
                        break;
                    case 4:
                        score_ = (level_.left[item.c_str()].GetDouble() == level_.right[item.c_str()].GetDouble());
                        break;
                    case 5:
                        score_ = (level_.left[item.c_str()].GetBool() == level_.right[item.c_str()].GetBool());
                        break;
                    case 6:
                        score_ = 1;
                        break;
                    default:
                        score_ = 0;
                        break;
                }
                score += score_;
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