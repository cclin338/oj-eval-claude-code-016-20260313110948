#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_SIZE = 65;
const int ORDER = 100; // B+ tree order

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

struct Node {
    bool is_leaf;
    int num_keys;
    Entry keys[ORDER];
    int children[ORDER + 1];
    int next; // For leaf nodes, link to next leaf

    Node() : is_leaf(true), num_keys(0), next(-1) {
        memset(children, -1, sizeof(children));
    }
};

class BPlusTree {
private:
    string filename;
    fstream file;
    int root_pos;
    int node_count;

    void read_node(int pos, Node& node) {
        file.seekg(pos);
        file.read((char*)&node, sizeof(Node));
    }

    void write_node(int pos, const Node& node) {
        file.seekp(pos);
        file.write((const char*)&node, sizeof(Node));
        file.flush();
    }

    int allocate_node() {
        return node_count++;
    }

    void insert_in_leaf(Node& node, const Entry& entry) {
        int i = node.num_keys - 1;
        while (i >= 0 && entry < node.keys[i]) {
            node.keys[i + 1] = node.keys[i];
            i--;
        }
        node.keys[i + 1] = entry;
        node.num_keys++;
    }

    int split_leaf(int pos, Node& node, Entry& up_entry) {
        int new_pos = allocate_node();
        Node new_node;
        new_node.is_leaf = true;

        int mid = node.num_keys / 2;
        new_node.num_keys = node.num_keys - mid;
        for (int i = 0; i < new_node.num_keys; i++) {
            new_node.keys[i] = node.keys[mid + i];
        }
        node.num_keys = mid;

        new_node.next = node.next;
        node.next = new_pos;

        up_entry = new_node.keys[0];

        write_node(pos, node);
        write_node(new_pos * sizeof(Node), new_node);

        return new_pos;
    }

    int split_internal(int pos, Node& node, Entry& up_entry) {
        int new_pos = allocate_node();
        Node new_node;
        new_node.is_leaf = false;

        int mid = node.num_keys / 2;
        up_entry = node.keys[mid];

        new_node.num_keys = node.num_keys - mid - 1;
        for (int i = 0; i < new_node.num_keys; i++) {
            new_node.keys[i] = node.keys[mid + 1 + i];
            new_node.children[i] = node.children[mid + 1 + i];
        }
        new_node.children[new_node.num_keys] = node.children[node.num_keys];
        node.num_keys = mid;

        write_node(pos, node);
        write_node(new_pos * sizeof(Node), new_node);

        return new_pos;
    }

    bool insert_recursive(int pos, const Entry& entry, Entry& up_entry, int& new_child) {
        Node node;
        read_node(pos, node);

        if (node.is_leaf) {
            // Check if entry already exists
            for (int i = 0; i < node.num_keys; i++) {
                if (node.keys[i] == entry) {
                    return false; // Duplicate
                }
            }

            insert_in_leaf(node, entry);

            if (node.num_keys >= ORDER) {
                new_child = split_leaf(pos, node, up_entry);
                return true;
            } else {
                write_node(pos, node);
                return false;
            }
        } else {
            int i = 0;
            while (i < node.num_keys && !(entry < node.keys[i])) {
                i++;
            }

            Entry child_up_entry;
            int child_new;
            bool split = insert_recursive(node.children[i] * sizeof(Node), entry, child_up_entry, child_new);

            if (!split) return false;

            // Insert child_up_entry into this node
            for (int j = node.num_keys; j > i; j--) {
                node.keys[j] = node.keys[j - 1];
                node.children[j + 1] = node.children[j];
            }
            node.keys[i] = child_up_entry;
            node.children[i + 1] = child_new;
            node.num_keys++;

            if (node.num_keys >= ORDER) {
                new_child = split_internal(pos, node, up_entry);
                return true;
            } else {
                write_node(pos, node);
                return false;
            }
        }
    }

    void find_recursive(int pos, const char* index, vector<int>& results) {
        Node node;
        read_node(pos, node);

        if (node.is_leaf) {
            for (int i = 0; i < node.num_keys; i++) {
                if (strcmp(node.keys[i].index, index) == 0) {
                    results.push_back(node.keys[i].value);
                }
            }
        } else {
            int i = 0;
            Entry target;
            strncpy(target.index, index, MAX_KEY_SIZE - 1);
            target.index[MAX_KEY_SIZE - 1] = '\0';
            target.value = 0;

            while (i < node.num_keys && !(target < node.keys[i])) {
                i++;
            }
            find_recursive(node.children[i] * sizeof(Node), index, results);

            // Check next child if index might extend there
            while (i < node.num_keys && strcmp(node.keys[i].index, index) == 0) {
                i++;
                if (i <= node.num_keys) {
                    find_recursive(node.children[i] * sizeof(Node), index, results);
                }
            }
        }
    }

    bool delete_recursive(int pos, const Entry& entry) {
        Node node;
        read_node(pos, node);

        if (node.is_leaf) {
            int i = 0;
            while (i < node.num_keys && !(node.keys[i] == entry)) {
                i++;
            }

            if (i >= node.num_keys) return false;

            for (int j = i; j < node.num_keys - 1; j++) {
                node.keys[j] = node.keys[j + 1];
            }
            node.num_keys--;
            write_node(pos, node);
            return true;
        } else {
            int i = 0;
            while (i < node.num_keys && !(entry < node.keys[i])) {
                i++;
            }
            return delete_recursive(node.children[i] * sizeof(Node), entry);
        }
    }

public:
    BPlusTree(const string& fname) : filename(fname), root_pos(0), node_count(1) {
        file.open(filename, ios::in | ios::out | ios::binary);

        if (!file.is_open()) {
            // Create new file
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);

            Node root;
            write_node(0, root);
        }
    }

    ~BPlusTree() {
        if (file.is_open()) {
            file.close();
        }
    }

    void insert(const char* index, int value) {
        Entry entry(index, value);
        Entry up_entry;
        int new_child;

        bool split = insert_recursive(root_pos, entry, up_entry, new_child);

        if (split) {
            Node old_root;
            read_node(root_pos, old_root);

            int new_root_pos = allocate_node();
            Node new_root;
            new_root.is_leaf = false;
            new_root.num_keys = 1;
            new_root.keys[0] = up_entry;
            new_root.children[0] = root_pos / sizeof(Node);
            new_root.children[1] = new_child;

            write_node(new_root_pos * sizeof(Node), new_root);
            root_pos = new_root_pos * sizeof(Node);
        }
    }

    vector<int> find(const char* index) {
        vector<int> results;
        find_recursive(root_pos, index, results);
        sort(results.begin(), results.end());
        return results;
    }

    void del(const char* index, int value) {
        Entry entry(index, value);
        delete_recursive(root_pos, entry);
    }
};

int main() {
    BPlusTree tree("data.db");

    int n;
    cin >> n;

    string cmd;
    char index[MAX_KEY_SIZE];
    int value;

    for (int i = 0; i < n; i++) {
        cin >> cmd;

        if (cmd == "insert") {
            cin >> index >> value;
            tree.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            tree.del(index, value);
        } else if (cmd == "find") {
            cin >> index;
            vector<int> results = tree.find(index);
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
