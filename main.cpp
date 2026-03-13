#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

using namespace std;

const int MAX_KEY_SIZE = 65;

struct Entry {
    char index[MAX_KEY_SIZE];
    int value;

    Entry() { memset(index, 0, sizeof(index)); value = 0; }
    Entry(const char* idx, int val) {
        strncpy(index, idx, MAX_KEY_SIZE - 1);
        index[MAX_KEY_SIZE - 1] = '\0';
        value = val;
    }

    bool operator<(const Entry& other) const {
        int cmp = strcmp(index, other.index);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }

    bool operator==(const Entry& other) const {
        return strcmp(index, other.index) == 0 && value == other.value;
    }
};

class Database {
private:
    string filename;
    map<string, set<int>> data;

    void load() {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) return;

        int count;
        if (!file.read((char*)&count, sizeof(count))) return;

        for (int i = 0; i < count; i++) {
            Entry entry;
            if (!file.read((char*)&entry, sizeof(Entry))) break;
            data[string(entry.index)].insert(entry.value);
        }
        file.close();
    }

    void save() {
        ofstream file(filename, ios::binary | ios::trunc);
        if (!file.is_open()) return;

        int count = 0;
        for (const auto& pair : data) {
            count += pair.second.size();
        }

        file.write((const char*)&count, sizeof(count));

        for (const auto& pair : data) {
            for (int value : pair.second) {
                Entry entry(pair.first.c_str(), value);
                file.write((const char*)&entry, sizeof(Entry));
            }
        }

        file.close();
    }

public:
    Database(const string& fname) : filename(fname) {
        load();
    }

    ~Database() {
        save();
    }

    void insert(const char* index, int value) {
        data[string(index)].insert(value);
    }

    vector<int> find(const char* index) {
        vector<int> results;
        auto it = data.find(string(index));
        if (it != data.end()) {
            results.assign(it->second.begin(), it->second.end());
        }
        return results;
    }

    void del(const char* index, int value) {
        auto it = data.find(string(index));
        if (it != data.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                data.erase(it);
            }
        }
    }
};

int main() {
    Database db("data.db");

    int n;
    cin >> n;

    string cmd;
    char index[MAX_KEY_SIZE];
    int value;

    for (int i = 0; i < n; i++) {
        cin >> cmd;

        if (cmd == "insert") {
            cin >> index >> value;
            db.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            db.del(index, value);
        } else if (cmd == "find") {
            cin >> index;
            vector<int> results = db.find(index);
            if (results.empty()) {
                cout << "null" << endl;
            } else {
                for (size_t j = 0; j < results.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << results[j];
                }
                cout << endl;
            }
        }
    }

    return 0;
}
