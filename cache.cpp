#include <cstdlib>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "cache.h"
#include "response.h"

using namespace std;

void cache::saveCache(string req, char *r) {
  // unordered_map<string, string>::iterator pos = way.find(req);
  string resp = r;
  // cout << "to save" << endl;
  // cout << resp << endl;
  response curr(resp);
  // cout << "create response" << endl;
  // curr.parse_response();
  // cout << "parse response" << endl;
  string control = curr.get_cache_control();
  // cout << "control" << endl;
  // cout << control << endl;
  string expire_time = curr.get_expire_time();
  // cout << expire_time << endl;
  string curr_time = curr.get_date();
  // cout << curr_time << endl;
  // cannot be cached
  if (control.find("no cache") != string::npos ||
      control.find("no store") != string::npos) {
    cout << "not cacheable because " << control << endl;
    return;
  }
  // not 200 OK
  if (!curr.get_status()) {
    cout << "not cacheable because not 200 OK" << endl;
    return;
  }
  // evict by LRU
  if (size == cap) {
    string to_evict = way.begin()->first;
    string url = to_evict.substr(to_evict.find(' ') + 1);
    url = url.substr(0, url.find(' '));
    cout << "NOTE evicted " << url << " from cache" << endl;
    way.erase(way.begin());
    pair<string, string> curr = make_pair(req, resp);
    way.insert(curr);
    return;
  } else {
    // will expire
    if (!expire_time.empty()) {
      // need revalidate
      if (control.find("must-revalidate") != string::npos) {
        cout << "cached, but requires revalidation" << endl;
      }
      cout << "cached, but expires at " << expire_time << endl;
    } else {
      cout << "cached" << endl;
    }
    pair<string, string> curr = make_pair(req, resp);
    way.insert(curr);
    for (auto c : way) {
      cout << "in way: " << endl;
      cout << c.first << endl;
      cout << c.second << endl;
    }
    //    cout << req << endl;
    // cout << resp << "has been cached" << endl;
    size++;
    return;
  }
}

bool cache::getCache(string req, int client_fd) {
  // find req in cache
  for (auto c : way) {
    cout << "in way: " << endl;
    cout << c.first << endl;
    cout << c.second << endl;
  }
  if (way.find(req) != way.end()) {
    string resp = way[req];
    response curr(resp);
    // curr.parse_response();
    string control = curr.get_cache_control();
    if (control.find("must-revalidate") != string::npos) {
      cout << "in cache, requires validation" << endl;
      return false;
    }
    // never expires
    string expire_time = curr.get_expire_time();
    string curr_time = curr.get_date();
    if (expire_time.empty()) {
      cout << "in cache, valid" << endl;
      cout << "Responding HTTP/1.1 200 OK" << endl;
      //      send(client_fd, &resp, resp.size(), 0);
      int sbyte;
      int send_index = 0;
      while ((sbyte = send(client_fd, &resp[send_index], 100, 0)) == 100) {
        send_index += 100;
      }
      return true;
    } else {
      if (curr.check_expire()) {
        cout << "in cache, but expires at " << expire_time << endl;
        return false;
      } else {
        cout << "in cache, valid" << endl;
        //        send(client_fd, &resp, resp.size(), 0);
        int sbyte;
        int send_index = 0;
        // const char *r = resp.c_str();
        while ((sbyte = send(client_fd, &resp[send_index], 100, 0)) == 100) {
          send_index += 100;
        }
        cout << resp << endl;
        close(client_fd);
        return true;
      }
    }
  } else {
    cout << "not in cache" << endl;
    return false;
  }
}
