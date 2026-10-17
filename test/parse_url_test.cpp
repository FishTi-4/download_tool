
#include <string>
#include <iostream>

using namespace std;
std::pair<std::string, std::string> parse_url(std::string url){
      int pos = url.find("://");
      if (pos == std::string::npos) pos = 0;
      else  pos += 3;

      int cut = url.find("/", pos);

      if(cut == std::string::npos) return {url.substr(pos), ""};
      else return {url.substr(pos, cut - pos), url.substr(cut + 1)};
    }
int main(){

  cout << parse_url("www.baidu.com/").first << "|" << endl;
  
  cout << parse_url("www.baidu.com/").second << "|" << endl;

}