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

Linus::jsondiff::TreeLevel::TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input, const std::string& path_left, const std::string& path_right, const std::string& up_level) : left(left_input), right(right_input), left_path(path_left), right_path(path_right), up(up_level)
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

/*void Linus::jsondiff::JsonDiffer::parallel_diff_level(std::queue<std::pair<unsigned int, unsigned int>>& work_queue, std::vector<std::vector<double>>& dp, Linus::jsondiff::TreeLevel& level, std::mutex& work_queue_mutex, std::mutex& dp_mutex)
{
    bool ctn = true;
    int volumn = 6;
    std::chrono::duration<double> work_queue_eplapsed(0);
    //std::chrono::duration<double> tree_eplapsed(0);
    std::chrono::duration<double> diff_eplapsed(0);
    std::chrono::duration<double> dp_eplapsed(0);
    while (ctn)
    {
        std::vector<std::pair<unsigned int, unsigned int>> task_list(volumn);
        int count = 0;
        {
            auto lock_start = std::chrono::high_resolution_clock::now();
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
            auto lock_finish = std::chrono::high_resolution_clock::now();
            work_queue_eplapsed += (lock_finish - lock_start);
            //std::cout << std::to_string(task.first) << ":" << std::to_string(task.second) << std::endl;
        }
        
        std::vector<double> score_(volumn);
        for (int i = 0; i < count; ++i)
        {
            //auto lock_start = std::chrono::high_resolution_clock::now();
            //Linus::jsondiff::TreeLevel level_(level.left[task_list[i].first], level.right[task_list[i].second], level.left_path + "[" + std::to_string(task_list[i].first) + "]", level.right_path + "[" + std::to_string(task_list[i].second) + "]", level.left_path);
            //auto lock_finish = std::chrono::high_resolution_clock::now();
            //tree_eplapsed += (lock_finish - lock_start);
            auto lock_start = std::chrono::high_resolution_clock::now();
            score_[i] = (level.left[task_list[i].first].GetInt() == level.right[task_list[i].second].GetInt());
            auto lock_finish = std::chrono::high_resolution_clock::now();
            diff_eplapsed += (lock_finish - lock_start);
        }
        {
            auto lock_start = std::chrono::high_resolution_clock::now();
            std::lock_guard<std::mutex> lock(dp_mutex);
            for (int i = 0; i < count; ++i)
            {
                dp[task_list[i].first + 1][task_list[i].second + 1] = score_[i];
            }
            auto lock_finish = std::chrono::high_resolution_clock::now();
            dp_eplapsed += (lock_finish - lock_start);
        }
    }
    {
        std::lock_guard<std::mutex> lock(work_queue_mutex);
        std::cout << "Thread: work_queue lock time: " << work_queue_eplapsed.count() << " s\n";
        //std::cout << "Thread: tree time: " << tree_eplapsed.count() << " s\n";
        std::cout << "Thread: diff time: " << diff_eplapsed.count() << " s\n";
        std::cout << "Thread: dp_table lock time: " << dp_eplapsed.count() << " s\n";
    }
}*/

/*void Linus::jsondiff::JsonDiffer::parallel_diff_level(std::queue<std::pair<unsigned int, unsigned int>>& work_queue, std::vector<std::vector<double>>& dp, Linus::jsondiff::TreeLevel& level, std::mutex& work_queue_mutex, std::mutex& dp_mutex)
{
    bool ctn = true;
    int volumn = 6;
    //std::chrono::duration<double> work_queue_eplapsed(0);
    //std::chrono::duration<double> tree_eplapsed(0);
    //std::chrono::duration<double> diff_eplapsed(0);
    //std::chrono::duration<double> dp_eplapsed(0);
    while (ctn)
    {
        std::vector<std::pair<unsigned int, unsigned int>> task_list(volumn);
        int count = 0;
        {
            //auto lock_start = std::chrono::high_resolution_clock::now();
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
            //auto lock_finish = std::chrono::high_resolution_clock::now();
            //work_queue_eplapsed += (lock_finish - lock_start);
            //std::cout << std::to_string(task.first) << ":" << std::to_string(task.second) << std::endl;
        }
        
        std::vector<double> score_(volumn);
        for (int i = 0; i < count; ++i)
        {
            //auto lock_start = std::chrono::high_resolution_clock::now();
            //Linus::jsondiff::TreeLevel level_(level.left[task_list[i].first], level.right[task_list[i].second], level.left_path + "[" + std::to_string(task_list[i].first) + "]", level.right_path + "[" + std::to_string(task_list[i].second) + "]", level.left_path);
            //auto lock_finish = std::chrono::high_resolution_clock::now();
            //tree_eplapsed += (lock_finish - lock_start);
            //auto lock_start = std::chrono::high_resolution_clock::now();
            std::string leftPath = "/number/" + std::to_string(task_list[i].first);
            std::string rightPath = "/number/" + std::to_string(task_list[i].second);
            const rapidjson::Value* leftValue = rapidjson::Pointer(leftPath.c_str()).Get(left);
            const rapidjson::Value* rightValue = rapidjson::Pointer(rightPath.c_str()).Get(right);
            score_[i] = (leftValue->GetInt() == rightValue->GetInt());
            //auto lock_finish = std::chrono::high_resolution_clock::now();
            //diff_eplapsed += (lock_finish - lock_start);
        }
        {
            //auto lock_start = std::chrono::high_resolution_clock::now();
            std::lock_guard<std::mutex> lock(dp_mutex);
            for (int i = 0; i < count; ++i)
            {
                dp[task_list[i].first + 1][task_list[i].second + 1] = score_[i];
            }
            //auto lock_finish = std::chrono::high_resolution_clock::now();
            //dp_eplapsed += (lock_finish - lock_start);
        }
    }
    {
        std::lock_guard<std::mutex> lock(work_queue_mutex);
        //std::cout << "Thread: work_queue lock time: " << work_queue_eplapsed.count() << " s\n";
        //std::cout << "Thread: tree time: " << tree_eplapsed.count() << " s\n";
        //std::cout << "Thread: diff time: " << diff_eplapsed.count() << " s\n";
        //std::cout << "Thread: dp_table lock time: " << dp_eplapsed.count() << " s\n";
    }
}*/

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

    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Retreiving LCS: " << pLelapsed.count() << " s\n";

    return pair_list;
}

std::map<unsigned int, unsigned int> Linus::jsondiff::JsonDiffer::LCS(Linus::jsondiff::TreeLevel level)
{
    unsigned int len_left = level.left.Size();
    unsigned int len_right = level.right.Size();
    std::vector<int> type_left(len_left);
    std::vector<int> type_right(len_right);
    std::vector<std::vector<double>> dp(len_left + 1, std::vector<double>(len_right + 1, 0.0));

    auto pLstart = std::chrono::high_resolution_clock::now();
    for (unsigned int i = 0; i < len_left; ++i)
    {
        type_left[i] =  Linus::jsondiff::JsonDiffer::get_type(level.left[i]);
    }
    for (unsigned int j = 0; j < len_right; ++j)
    {
        type_right[j] = Linus::jsondiff::JsonDiffer::get_type(level.right[j]);
    }
    auto pLfinish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> pLelapsed = pLfinish - pLstart;
    std::cout << "Single thread: Getting types of array elements: " << pLelapsed.count() << " s\n";
    //std::chrono::duration<double> value_time(0);
    //std::chrono::duration<double> path_time(0); 
    //std::chrono::duration<double> tree_time(0);
    //std::chrono::duration<double> compute_time(0);    
    pLstart = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 1; i <= len_left; ++i)
    {
        for (unsigned int j = 1; j <= len_right; ++j)
        {
            //auto compute_start = std::chrono::high_resolution_clock::now();
            double score_;
            if (type_left[i-1] == type_right[j-1])
            {
                switch (type_left[i-1])
                {
                    case 0:
                    {
                        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
                        score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                        break;
                    }
                    case 1:
                    {
                        Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
                        score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                        break;
                    }
                    case 2:
                        score_ = (level.left[i-1].GetString() == level.right[j-1].GetString());
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
            //double score_ = (level.left[i-1].GetInt() == level.right[j-1].GetInt());
            //auto compute_finish = std::chrono::high_resolution_clock::now();
            //compute_time += (compute_finish - compute_start);
            //auto start = std::chrono::high_resolution_clock::now();
            //const rapidjson::Value& level_left = level.left[i - 1];
            //const rapidjson::Value& level_right = level.right[j - 1];
            //auto finish = std::chrono::high_resolution_clock::now();
            //value_time += (finish - start);

            //start = std::chrono::high_resolution_clock::now();
            //std::string leftpath = level.left_path + "[" + std::to_string(i - 1) + "]";
            //std::string rightpath = level.left_path + "[" + std::to_string(j - 1) + "]";
            //finish = std::chrono::high_resolution_clock::now();
            //path_time += (finish - start);

            //start = std::chrono::high_resolution_clock::now();
            //Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
            //Linus::jsondiff::TreeLevel level_(level_left, level_right, leftpath, rightpath, level.left_path);
            //finish = std::chrono::high_resolution_clock::now();
            //tree_time += (finish - start);

            //start = std::chrono::high_resolution_clock::now();
            //double score_ = _diff_level(level_, true);
            //finish = std::chrono::high_resolution_clock::now();
            //compute_time += (finish - start);

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
    //std::cout << "Single thread: copy rapidjson::Value&: " << value_time.count() << " s\n";
    //std::cout << "Single thread: combine path std::string: " << path_time.count() << " s\n";
    //std::cout << "Single thread: building TreeLevel time: " << tree_time.count() << " s\n";
    //std::cout << "Single thread: computing time: " << compute_time.count() << " s\n";
    pLfinish = std::chrono::high_resolution_clock::now();
    pLelapsed = pLfinish - pLstart;
    std::cout << "Single thread: computing dp table: " << pLelapsed.count() << " s\n";

    pLstart = std::chrono::high_resolution_clock::now();

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
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
                    score_ = Linus::jsondiff::JsonDiffer::compare_object(level_, true);
                    break;
                }
                case 1:
                {
                    Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
                    score_ = Linus::jsondiff::JsonDiffer::compare_array(level_, true);
                    break;
                }
                case 2:
                    score_ = (level.left[i-1].GetString() == level.right[j-1].GetString());
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
        //Linus::jsondiff::TreeLevel level_(level.left[i - 1], level.right[j - 1], level.left_path + "[" + std::to_string(i - 1) + "]", level.right_path + "[" + std::to_string(j - 1) + "]", level.left_path);
        //double score_ = _diff_level(level_, true);
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
    std::cout << "Retreive LCS: " << pLelapsed.count() << " s\n";

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
    if (level.left.GetString() != level.right.GetString())
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
    Linus::jsondiff::TreeLevel root_level(left, right, "", "", "");
    return Linus::jsondiff::JsonDiffer::diff_level(root_level, false) == 1.0;
}


