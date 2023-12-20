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

void run(std::string left, std::string right, bool advanced_mode, double similarity_threshold)
{
    try 
    {
        rapidjson::Document left_json_ = loadjson(left);
        rapidjson::Document right_json_ = loadjson(right);
        const rapidjson::Value& left_json = left_json_;
        //cout << Linus::jsondiff::ValueToString(left_json) << endl;
        const rapidjson::Value& right_json = right_json_;
        //cout << Linus::jsondiff::ValueToString(right_json) << endl;
        Linus::jsondiff::JsonDiffer jsondiffer(left_json, right_json, advanced_mode, similarity_threshold);
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
    std::string left, right;
    bool advanced_mode = false;
    double similarity_threshold = 0.5;
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
    }
    run(left, right, advanced_mode, similarity_threshold);
    return 0;
}


/*int main ()
{
    std::string json1 = "{\"person\":[1, {\"name\":\"Peter\"},3,4]}";
    std::string json2 = "{\"person\":[1, {\"name\":\"Linus\"},3,4]}";
    cout << json1 << endl;
    cout << json2 << endl;
    run(json1, json2, false);
    return 0;
}*/