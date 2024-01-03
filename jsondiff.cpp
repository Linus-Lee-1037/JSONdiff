#include "document.h"

rapidjson::Document loadjson(std::string json)
{
    rapidjson::Document document;
    if (json[0] == '{')
    {
        document.Parse(json.c_str());
    }
    else
    {
        std::ifstream file(json);
        if (!file.is_open())
        {
            std::cerr << "Cannot open file: " << json << std::endl;
            throw std::runtime_error("File open failed");
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        document.Parse(buffer.str().c_str());
    }
    return document;
}

void PrintRecords(std::map<std::string, std::vector<std::string>> records)
{
    for (const auto& pair : records)
    {
        for (const auto& val : pair.second)
        {
            std::cout << pair.first << ": " << val << std::endl;
        }
    }
}

void run(std::string left, std::string right, bool advanced_mode, double similarity_threshold, int thread_count)
{
    try 
    {
        rapidjson::Document left_json_ = loadjson(left);
        rapidjson::Document right_json_ = loadjson(right);
        const rapidjson::Value& left_json = left_json_;
        //cout << Linus::jsondiff::ValueToString(left_json) << endl;
        const rapidjson::Value& right_json = right_json_;
        //cout << Linus::jsondiff::ValueToString(right_json) << endl;
        Linus::jsondiff::JsonDiffer jsondiffer(left_json, right_json, advanced_mode, similarity_threshold, thread_count);
        bool same = jsondiffer.diff();
        std::string result = same ? "Same" : "Different";
        std::cout << result << std::endl;
        PrintRecords(jsondiffer.records);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char * argv[])
{
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string left, right;
    bool advanced_mode = false;
    double similarity_threshold = 0.5;
    int thread_count = 1;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-left" && i + 1 < argc)
        {
            left = argv[++i];
            //cout << left << endl;
        }
        if (arg == "-right" && i + 1 < argc)
        {
            right = argv[++i];
            //cout << right << endl;
        }
        if (arg == "-advanced_mode" || arg == "-A")
        {
            advanced_mode = true;
        }
        if ((arg == "-similarity_threshold" || arg == "-S") && i + 1 < argc)
        {
            try 
            {
                similarity_threshold = std::stod(argv[++i]);
            } 
            catch (const std::invalid_argument& e) 
            {
                std::cerr << "Invalid similarity threshold: " << argv[i] << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::cerr << "Similarity threshold out of range: " << argv[i] << std::endl;
            }
        }
        if ((arg == "-nthreads" || arg == "-N") && i + 1 < argc)
        {
            try 
            {
                thread_count = std::stoi(argv[++i]);
            } 
            catch (const std::invalid_argument& e) 
            {
                std::cerr << "Invalid thread count: " << argv[i] << std::endl;
            }
            catch (const std::out_of_range& e)
            {
                std::cerr << "Thread cound out of range: " << argv[i] << std::endl;
            }
        }
    }
    run(left, right, advanced_mode, similarity_threshold, thread_count);
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Total time: " << elapsed.count() << " s\n";
    return 0;
}


/*int main ()
{
    //std::string json1 = "{\"number\":[1,2,3,4,5]}";
    //std::string json2 = "{\"number\":[1,2,4,5]}";
    std::string json1 = ".\\left2.json";
    std::string json2 = ".\\right2.json";
    run(json1, json2, true, 0.5, 6);
    return 0;
}*/