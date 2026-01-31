#include <optional>
#include <thread>
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <fstream>
#include <cstdio>
using json = nlohmann::json;
int main(){
    std::string response_string;
    CURL* curl = curl_easy_init();
    std::string latest_version;
    std::string download_url;
    const char* jar_file = "/home/minecraft/velocity/plugins/Geyser.jar"; //REPLACE WITH ACTUAL PATH
    const char* version_file = "/home/minecraft/velocity/plugins/geyser.txt"; //REPLACE WITH PATH OF VERSION TXT FILE
    if (!curl){
            std::cerr << "Error: Failed to initialize curl.\n";
            return 1;
    }
    while(true){
        response_string.clear();
        std::optional<std::string> result;
        std::ifstream file(version_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot read file.\n";
            return 1;
        }
        std::string line;
        if (!std::getline(file, line)) {
            std::cout << "File is empty!\n";
            return 1;
        }
        result = std::move(line);
        file.close();
        response_string.clear();
    	curl_easy_setopt(curl, CURLOPT_URL, "https://api.modrinth.com/v2/project/geyser/version?loaders=%5B%22velocity%22%5D");
    	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,+[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {std::string* s = static_cast<std::string*>(userp);s->append(static_cast<char*>(contents), size * nmemb);return size * nmemb;});
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    	curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl-json-downloader/1.0");
    	CURLcode res = curl_easy_perform(curl);
    	if (res != CURLE_OK){
        	std::cerr<<"curl_easy_perform() failed: "<<curl_easy_strerror(res)<<"\n";
        	return 1;
    	}
    	try{
        	json j = json::parse(response_string);
        	if (!j.is_array() || j.empty()){
            	std::cerr << "Invalid or empty versions array";
            	continue;
        	}
            auto latest = j[0];
            latest_version = latest.value("version_number", latest.value("name", ""));
            if (latest.contains("files") && latest["files"].is_array() && !latest["files"].empty()) {
                for (const auto& file : latest["files"]) {
                    if (file.value("primary", false) && file.contains("url") && file["url"].is_string()) {
                        download_url = file["url"].get<std::string>();
                        break;
                    }
                }
                if (download_url.empty() && latest["files"][0].contains("url") && latest["files"][0]["url"].is_string()) {
                    download_url = latest["files"][0]["url"].get<std::string>();
                }
            }
            if (download_url.empty()) {
                std::cerr << "No download URL found!\n";
                std::this_thread::sleep_for(std::chrono::seconds(60));
                continue;
            }
    	}
    	catch (const json::parse_error& e){
        	std::cerr << "JSON parse error!\n"<< "  what:  " << e.what() << "\n"<< "  byte:  " << e.byte << "\n"<< "  id:    " << e.id << "\n";
        	return 1;
    	}
        catch (const std::exception& e){
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        if(latest_version==result.value()){
            std::cout<<"Geyser-Velocity is already up to date!\n";
        }else{
            std::cout<<"Found new Geyser-Velocity version: "<<latest_version<<" Current version: "<<result.value()<<std::endl;
            if(std::remove(jar_file)){
                std::cout<<"Failed to delete original jar. \n";
            }
            FILE* fp = fopen(jar_file, "wb");
            curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,+[](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {return fwrite(ptr, size, nmemb, static_cast<FILE*>(userdata));});
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_perform(curl);
            fclose(fp);
            std::ofstream out(version_file, std::ios::trunc);
            if (out.is_open()) {
                out << latest_version << "\n";
                out.close();
                std::cout << "Updated to latest version\n";
            } else {
                std::cerr << "Failed to write version.txt\n";
                return 1;
            }
        }
    	std::this_thread::sleep_for(std::chrono::seconds(60)); //REPLACE WITH INTERVAL THAT YOU WANT
    }
}
