
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <ctime>
#include <limits>
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

struct Account {
    string id;
    string name;
    string pin;
    long long balance = 0;
    bool active = true;
};

struct Transaction {
    string id;
    string type;
    string from;
    string to;
    long long amount;
    string timestamp;
};

class BankSystem {
private:
    unordered_map<string, Account> accounts;
    vector<Transaction> transactions;
    string dataFile = "data/bank_data.txt";
    int nextAccount = 1001;
    int nextTransaction = 1;

    string getTime() {
        time_t now = time(nullptr);
        tm* t = localtime(&now);
        stringstream ss;
        ss << put_time(t, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    string generateAccountID() {
        return "BK" + to_string(nextAccount++);
    }

    string generateTransactionID() {
        return "TXN" + to_string(nextTransaction++);
    }

    bool saveData() {
        try {
            fs::create_directories("data");

            ofstream file(dataFile + ".tmp");
            if (!file) return false;

            file << accounts.size() << '\n';

            for (const auto& [id, a] : accounts) {
                file << quoted(a.id) << ' '
                     << quoted(a.name) << ' '
                     << quoted(a.pin) << ' '
                     << a.balance << ' '
                     << a.active << '\n';
            }

            file << transactions.size() << '\n';

            for (const auto& t : transactions) {
                file << quoted(t.id) << ' '
                     << quoted(t.type) << ' '
                     << quoted(t.from) << ' '
                     << quoted(t.to) << ' '
                     << t.amount << ' '
                     << quoted(t.timestamp) << '\n';
            }

            file.close();
            if (!file) return false;

            fs::rename(dataFile + ".tmp", dataFile);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void loadData() {
        ifstream file(dataFile);
        if (!file) return;

        size_t count;
        if (!(file >> count)) return;

        for (size_t i = 0; i < count; i++) {
            Account a;
            file >> quoted(a.id)
                 >> quoted(a.name)
                 >> quoted(a.pin)
                 >> a.balance
                 >> a.active;

            if (!file) return;

            accounts[a.id] = a;

            if (a.id.size() > 2 && a.id.substr(0, 2) == "BK") {
                try {
                    nextAccount = max(
                        nextAccount,
                        stoi(a.id.substr(2)) + 1
                    );
                } catch (...) {}
            }
        }

        if (!(file >> count)) return;

        for (size_t i = 0; i < count; i++) {
            Transaction t;
            file >> quoted(t.id)
                 >> quoted(t.type)
                 >> quoted(t.from)
                 >> quoted(t.to)
                 >> t.amount
                 >> quoted(t.timestamp);

            if (!file) return;

            transactions.push_back(t);

            if (t.id.size() > 3 &&
                t.id.substr(0, 3) == "TXN") {
                try {
                    nextTransaction = max(
                        nextTransaction,
                        stoi(t.id.substr(3)) + 1
                    );
                } catch (...) {}
            }
        }
    }

    bool recordTransaction(
        const string& type,
        const string& from,
        const string& to,
        long long amount
    ) {
        transactions.push_back({
            generateTransactionID(),
            type,
            from,
            to,
            amount,
            getTime()
        });

        if (!saveData()) {
            transactions.pop_back();
            --nextTransaction;
            return false;
        }

        return true;
    }

    string readLine(const string& prompt) {
        cout << prompt;
        string value;
        getline(cin, value);
        return value;
    }

    long long readAmount() {
        string input = readLine("Enter amount (INR): ");

        try {
            size_t pos;
            long long amount = stoll(input, &pos);

            if (pos != input.size() || amount <= 0)
                throw invalid_argument("Invalid amount");

            return amount;
        }
        catch (...) {
            cout << "Invalid amount. Enter a positive whole number.\n";
            return -1;
        }
    }

    void showHeader() {
        cout << "\n====================================\n";
        cout << "       BANKCORE BANKING SYSTEM\n";
        cout << "====================================\n";
    }

public:
    BankSystem() {
        loadData();
    }

    void createAccount() {
        showHeader();
        cout << "\n--- CREATE NEW ACCOUNT ---\n";

        string name = readLine("Customer name: ");
        string pin = readLine("Create 4-digit PIN: ");

        if (name.empty() || pin.size() != 4 ||
            !all_of(pin.begin(), pin.end(),
                    [](unsigned char c) { return isdigit(c); })) {
            cout << "Invalid name or PIN.\n";
            return;
        }

        Account a;
        a.id = generateAccountID();
        a.name = name;
        a.pin = pin;
        a.balance = 0;

        accounts[a.id] = a;

        if (!saveData()) {
            accounts.erase(a.id);
            --nextAccount;
            cout << "Account creation failed. Storage error.\n";
            return;
        }

        cout << "\nAccount created successfully!\n";
        cout << "Your Account ID: " << a.id << '\n';
        cout << "Please remember your account ID.\n";
    }

    void login() {
        showHeader();
        cout << "\n--- CUSTOMER LOGIN ---\n";

        string id = readLine("Account ID: ");
        string pin = readLine("Enter PIN: ");

        auto it = accounts.find(id);

        if (it == accounts.end() ||
            !it->second.active ||
            it->second.pin != pin) {
            cout << "Invalid credentials or inactive account.\n";
            return;
        }

        cout << "\nWelcome, " << it->second.name << "!\n";
        customerMenu(id);
    }

    void checkBalance(const string& id) {
        cout << "\nAvailable balance: INR "
             << accounts.at(id).balance << '\n';
    }

    void deposit(const string& id) {
        long long amount = readAmount();
        if (amount <= 0) return;

        auto& a = accounts.at(id);

        if (amount > numeric_limits<long long>::max() - a.balance) {
            cout << "Amount exceeds supported balance limit.\n";
            return;
        }

        a.balance += amount;

        if (!recordTransaction("DEPOSIT", id, "-", amount)) {
            a.balance -= amount;
            cout << "Transaction failed. Changes rolled back.\n";
            return;
        }

        cout << "Deposit successful!\n";
        checkBalance(id);
    }

    void withdraw(const string& id) {
        long long amount = readAmount();
        if (amount <= 0) return;

        auto& a = accounts.at(id);

        if (amount > a.balance) {
            cout << "Insufficient balance!\n";
            return;
        }

        a.balance -= amount;

        if (!recordTransaction("WITHDRAW", id, "-", amount)) {
            a.balance += amount;
            cout << "Transaction failed. Changes rolled back.\n";
            return;
        }

        cout << "Withdrawal successful!\n";
        checkBalance(id);
    }

    void transfer(const string& id) {
        string receiver = readLine("Receiver Account ID: ");

        if (receiver == id ||
            !accounts.count(receiver) ||
            !accounts.at(receiver).active) {
            cout << "Invalid receiver account.\n";
            return;
        }

        long long amount = readAmount();
        if (amount <= 0) return;

        auto& sender = accounts.at(id);
        auto& recipient = accounts.at(receiver);

        if (amount > sender.balance) {
            cout << "Insufficient balance!\n";
            return;
        }

        if (amount > numeric_limits<long long>::max()
                     - recipient.balance) {
            cout << "Receiver balance limit exceeded.\n";
            return;
        }

        sender.balance -= amount;
        recipient.balance += amount;

        if (!recordTransaction("TRANSFER", id, receiver, amount)) {
            sender.balance += amount;
            recipient.balance -= amount;
            cout << "Transfer failed. Changes rolled back.\n";
            return;
        }

        cout << "Transfer completed successfully!\n";
        checkBalance(id);
    }

    void transactionHistory(const string& id) {
        cout << "\n========== TRANSACTION HISTORY ==========\n";

        bool found = false;

        for (const auto& t : transactions) {
            if (t.from == id || t.to == id) {
                found = true;

                cout << "\nTransaction ID: " << t.id;
                cout << "\nType: " << t.type;
                cout << "\nFrom: " << t.from;
                cout << "\nTo: " << t.to;
                cout << "\nAmount: INR " << t.amount;
                cout << "\nDate: " << t.timestamp << '\n';
                cout << "-----------------------------------------\n";
            }
        }

        if (!found)
            cout << "No transactions found.\n";
    }

    void customerMenu(const string& id) {
        int choice;

        do {
            cout << "\n========== CUSTOMER DASHBOARD ==========\n";
            cout << "1. Check Balance\n";
            cout << "2. Deposit Money\n";
            cout << "3. Withdraw Money\n";
            cout << "4. Transfer Money\n";
            cout << "5. Transaction History\n";
            cout << "6. Account Details\n";
            cout << "0. Logout\n";
            cout << "========================================\n";

            string input = readLine("Select option: ");

            try {
                size_t pos;
                choice = stoi(input, &pos);
                if (pos != input.size()) choice = -1;
            }
            catch (...) {
                choice = -1;
            }

            switch (choice) {
                case 1:
                    checkBalance(id);
                    break;

                case 2:
                    deposit(id);
                    break;

                case 3:
                    withdraw(id);
                    break;

                case 4:
                    transfer(id);
                    break;

                case 5:
                    transactionHistory(id);
                    break;

                case 6: {
                    const auto& a = accounts.at(id);
                    cout << "\nAccount ID: " << a.id
                         << "\nCustomer: " << a.name
                         << "\nBalance: INR " << a.balance
                         << "\nStatus: Active\n";
                    break;
                }

                case 0:
                    cout << "Logged out successfully.\n";
                    break;

                default:
                    cout << "Invalid option!\n";
            }

        } while (choice != 0);
    }

    void run() {
        int choice;

        do {
            showHeader();

            cout << "\n1. Create Account\n";
            cout << "2. Customer Login\n";
            cout << "0. Exit\n";

            string input = readLine("\nEnter choice: ");

            try {
                size_t pos;
                choice = stoi(input, &pos);
                if (pos != input.size()) choice = -1;
            }
            catch (...) {
                choice = -1;
            }

            switch (choice) {
                case 1:
                    createAccount();
                    break;

                case 2:
                    login();
                    break;

                case 0:
                    cout << "Thank you for using BankCore!\n";
                    break;

                default:
                    cout << "Invalid choice!\n";
            }

        } while (choice != 0);
    }
};

int main() {
    BankSystem bank;
    bank.run();
    return 0;
}
