#pragma once

#include <string>
#include <unordered_map>

namespace download {

  struct HttpResponse {
    int status_code;
    std::unordered_map<std::string, std::string> headers;
    // std::string body;
  };

  HttpResponse parse_http(std::string s){
    HttpResponse ret;

    auto cut = [](const std::string& s) -> std::string {
      int start = s.find_first_not_of(" \t\r\n");
      int end = s.size();
      while(end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n')){
        end--;
      }
      return s.substr(start, end-start);
    };

    int p1 = s.find(' ');
    int p2 = s.find(' ', p1+1);

    ret.status_code = std::stoi(cut(s.substr(p1+1, p2-p1-1)));

    int ed = s.find("\r\n");
    while(true){ 
      ed += 2;
      int p = s.find(':', ed);
      if(p == std::string::npos)  break;

      std::string fi = cut(s.substr(ed, p-ed));
      ed = s.find("\r\n", ed);
      std::string se = cut(s.substr(p+1, ed-p));
      
      ret.headers[fi] = se;
    }

    return ret;
  }


} // namespace download
