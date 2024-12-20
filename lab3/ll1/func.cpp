#include "head.h"

// 非默认的构造函数
Grammar::Grammar(vector<Symbol> terminals, vector<Symbol> nonterminals, vector<Rule> rules)
    : terminals(terminals), nonterminals(nonterminals), rules(rules)
{
    this->terminals.push_back("ε");
    this->terminals.push_back("$");
}

// 输入文法
void Grammar::input()
{
    cout << "请输入文法规则：" << endl;
    string s;
    while (getline(cin, s))
    {
        if (s == "\n" || s == "")
            break;
        Rule r;
        Symbol left = s.substr(0, s.find("->"));
        r.left = left;
        s = s.substr(s.find("->") + 2);
        vector<Symbol> right_part;
        size_t pos = 0;
        while ((pos = s.find('|')) != string::npos)
        {
            string token = s.substr(0, pos);
            for (char &c : token)
            {
                Symbol sym = string(1, c);
                right_part.push_back(sym);
            }
            r.rights.push_back(right_part);
            right_part.clear();
            s.erase(0, pos + 1);
        }
        for (char &c : s)
        {
            Symbol sym = string(1, c);
            right_part.push_back(sym);
        }
        r.rights.push_back(right_part);
        rules.push_back(r);
    }
}

// 插入规则
bool Grammar::insert(const Rule &r)
{
    auto it = find(rules.begin(), rules.end(), r);

    // 如果规则左侧不在文法中，则插入规则
    if (it == rules.end())
    {
        rules.push_back(r);
        if (find(nonterminals.begin(), nonterminals.end(), r.left) == nonterminals.end())
            nonterminals.push_back(r.left);
        for (vector<Symbol> right : r.rights)
            for (Symbol s : right)
                if (find(terminals.begin(), terminals.end(), s) == terminals.end() &&
                    find(nonterminals.begin(), nonterminals.end(), s) == nonterminals.end())
                {
                    if (isupper(s[0]))
                        nonterminals.push_back(s);
                    else
                        terminals.push_back(s);
                }
        return true;
    }

    for (vector<Symbol> right : r.rights)
    {
        if (find(it->rights.begin(), it->rights.end(), right) == it->rights.end())
        {
            // 如果规则右侧不在文法中，则插入规则
            it->rights.push_back(right);
            // 更新文法的符号集合
            for (Symbol s : right)
                if (find(terminals.begin(), terminals.end(), s) == terminals.end() &&
                    find(nonterminals.begin(), nonterminals.end(), s) == nonterminals.end())
                {
                    if (isupper(s[0]))
                        nonterminals.push_back(s);
                    else
                        terminals.push_back(s);
                }
            return true;
        }
    }

    // 若规则已存在，则返回false
    return false;
}

// 合并文法，需要手动销毁被合并的函数
void Grammar::merge(Grammar *g)
{
    for (auto &t : g->terminals)
        if (find(terminals.begin(), terminals.end(), t) == terminals.end())
            terminals.push_back(t);

    for (auto &nt : g->nonterminals)
        if (find(nonterminals.begin(), nonterminals.end(), nt) == nonterminals.end())
            nonterminals.push_back(nt);

    for (auto &rule : g->rules)
    {
        auto it = find(rules.begin(), rules.end(), rule);
        if (it == rules.end())
            rules.push_back(rule);
        else
            for (vector<Symbol> right : rule.rights)
            {
                if (find(it->rights.begin(), it->rights.end(), right) == it->rights.end())
                    it->rights.push_back(right);
            }
    }
}

// 输出文法
void Grammar::output() const
{
    for (auto &r : rules)
    {
        cout << r.left << " -> ";
        for (size_t i = 0; i < r.rights.size(); ++i)
        {
            for (auto &s : r.rights[i])
            {
                cout << s << ' ';
            }
            if (i < r.rights.size() - 1)
                cout << "| ";
        }
        cout << endl;
    }
}

// 析构函数
Grammar::~Grammar()
{
    for (auto &r : rules)
    {
        for (auto &right : r.rights)
            right.clear();
        r.rights.clear();
    }
    rules.clear();
    terminals.clear();
    nonterminals.clear();
}

// 将非终结符按照字典序排序
void Grammar::sortNonterminals(bool desc = false)
{
    if (desc)
        sort(nonterminals.begin(), nonterminals.end(), greater<Symbol>());
    else
        sort(nonterminals.begin(), nonterminals.end(), less<Symbol>());
}

// 判断文法是否有直接左递归
bool hasLeftRecursion(const Rule &r)
{
    Symbol A = r.left;
    for (auto &right : r.rights)
    {
        if (right[0] == A)
            return true;
    }
    return false;
}

// 消除直接左递归
void Grammar::eliminateDirectLeftRecursion()
{
    vector<Rule> pre = rules;
    rules.clear();
    for (auto &r : pre)
    {
        if (!hasLeftRecursion(r))
        {
            rules.push_back(r);
            continue;
        }

        Symbol A = r.left;
        vector<vector<Symbol>> alpha; // alpha是所有以A开头的产生式
        vector<vector<Symbol>> beta;  // beta是所有不以A开头的产生式

        for (auto &right : r.rights)
        {
            if (right[0] == A)
                alpha.push_back(vector<Symbol>(right.begin() + 1, right.end()));
            else
                beta.push_back(right);
        }

        Symbol A1 = A + "'";
        while (find(nonterminals.begin(), nonterminals.end(), A1) != nonterminals.end())
            A1 += "'";
        nonterminals.push_back(A1);

        Rule newRule1;
        newRule1.left = A;
        for (vector<Symbol> &right : beta)
            if (right[0] != "ε")
                right.push_back(A1);
            else
                right = {A1};
        newRule1.rights = beta;
        rules.push_back(newRule1);

        Rule newRule2;
        newRule2.left = A1;
        for (vector<Symbol> &right : alpha)
            right.push_back(A1);
        newRule2.rights = alpha;
        newRule2.rights.push_back({"ε"});
        rules.push_back(newRule2);
    }
    pre.clear();
}

// 消除包含间接在内的所有左递归
void Grammar::eliminateLeftRecursion()
{
    sortNonterminals();
    for (auto i = nonterminals.begin(); i != nonterminals.end(); i++)
    {
        for (auto j = nonterminals.begin(); j != i; j++)
        {
            auto Ai = find_if(rules.begin(), rules.end(), [&](const Rule &r)
                              { return r.left == *i; });
            auto Aj = find_if(rules.begin(), rules.end(), [&](const Rule &r)
                              { return r.left == *j; });

            vector<vector<Symbol>> newRights;

            for (auto it = Ai->rights.begin(); it != Ai->rights.end();)
            {
                auto right = *it;
                if (!right.empty() && right[0] == *j)
                {
                    for (auto &ajRight : Aj->rights)
                    {
                        if (ajRight[0] == "ε")
                        {
                            vector<Symbol> newRight;
                            newRight.insert(newRight.end(), right.begin() + 1, right.end());
                            newRights.push_back(newRight);
                        }
                        else
                        {
                            vector<Symbol> newRight;
                            newRight.insert(newRight.end(), ajRight.begin(), ajRight.end());
                            newRight.insert(newRight.end(), right.begin() + 1, right.end());
                            newRights.push_back(newRight);
                        }
                    }
                    it = Ai->rights.erase(it);
                }
                else
                    ++it;
            }

            Ai->rights.insert(Ai->rights.end(), newRights.begin(), newRights.end());
        }
    }
    eliminateDirectLeftRecursion();
}

// 生成Trie树
TrieNode *generateTrie(const Rule &rule)
{
    // 生成Trie树
    TrieNode *root = new TrieNode();
    TrieNode *node;
    for (auto right : rule.rights)
    {
        node = root;
        for (auto symbol : right)
        {
            if (node->children.find(symbol) == node->children.end())
                node->children[symbol] = new TrieNode();
            node = node->children[symbol];
        }
        node->isEndOfWord = true;
    }
    return root;
}

// 使用递归构建无左公因式文法
void Grammar::traverseTrie(TrieNode *root, Symbol L, vector<Symbol> R)
{
    // 若当前节点只有一个子节点，则继续遍历树
    while (root->children.size() == 1)
    {
        auto it = root->children.begin();
        R.push_back(it->first);
        root = it->second;
    }
    // 若当前节点是叶子节点，则插入规则
    if (root->children.size() == 0 && root->isEndOfWord)
    {
        auto it = find(rules.begin(), rules.end(), Rule{L, {R}});
        if (it == rules.end())
            rules.push_back({L, {R}});
        else
            it->rights.push_back(R);
    }
    // 遇到分叉节点，将该节点作为新的根节点，继续遍历树
    else
    {
        Symbol newL = L;
        while (find(nonterminals.begin(), nonterminals.end(), newL) != nonterminals.end())
            newL += "'";
        nonterminals.push_back(newL);
        vector<Symbol> tmp = R;
        tmp.push_back(newL);

        auto it = find(rules.begin(), rules.end(), Rule{L, {tmp}});
        if (it == rules.end())
            rules.push_back({L, {tmp}});
        else
            it->rights.push_back(tmp);

        tmp.clear();
        for (auto it = root->children.begin(); it != root->children.end(); ++it)
        {
            tmp.push_back(it->first);
            traverseTrie(it->second, newL, tmp);
        }
        tmp.clear();
        if (root->isEndOfWord)
        {
            auto it = find(rules.begin(), rules.end(), Rule{newL, {{"ε"}}});
            it->rights.push_back({"ε"});
        }
    }
}

// 提取左公因式
void Grammar::extractLeftCommonFactors()
{

    vector<Rule> pre = rules;
    rules.clear();
    // 遍历每一条规则
    for (auto it = pre.begin(); it != pre.end(); ++it)
    {
        TrieNode *root = generateTrie(*it);
        Symbol L = it->left;
        vector<Symbol> R;

        for (auto it = root->children.begin(); it != root->children.end(); ++it)
        {
            R.push_back(it->first);
            traverseTrie(it->second, L, R);
            R.clear();
        }
    }
    pre.clear();
}

// 计算FIRST集
void Grammar::compute_FIRST(Symbol symbol, set<Symbol> &visited)
{
    if (visited.find(symbol) != visited.end())
        return;

    visited.insert(symbol);

    for (const auto &rule : rules)
    {
        if (rule.left == symbol)
            for (const auto &right : rule.rights)
                for (const auto &sym : right)
                    if (find(terminals.begin(), terminals.end(), sym) != terminals.end())
                    {
                        first[symbol].insert(sym);
                        break;
                    }
                    else
                    {
                        compute_FIRST(sym, visited);
                        bool hasEpsilon = false;
                        for (const auto &firstSym : first[sym])
                        {
                            if (firstSym == "ε")
                                hasEpsilon = true;
                            else
                                first[symbol].insert(firstSym);
                        }
                        if (!hasEpsilon)
                            break;
                    }
    }

    visited.erase(symbol);
}

// 获取FIRST集
FIRST Grammar::get_FIRST(bool display = false)
{
    if (!first.empty())
    {
        if (display)
            displaySet(first);
        return first;
    }
    set<Symbol> visited;

    for (const auto &nonterminal : nonterminals)
        compute_FIRST(nonterminal, visited);

    if (display)
        displaySet(first);
    return first;
}

// 输出FIRST或者FOLLOW集
void displaySet(const map<Symbol, set<Symbol>> &sets)
{
    for (const auto &entry : sets)
    {
        cout << "Set(" << entry.first << ") = { ";
        for (const auto &symbol : entry.second)
            cout << symbol << " ";
        cout << "}" << endl;
    }
}

void Grammar::compute_FOLLOW(Symbol symbol, set<Symbol> &visited, bool &changed)
{
    if (visited.find(symbol) != visited.end())
        return;

    visited.insert(symbol);

    for (const auto &rule : rules)
    {
        for (const auto &right : rule.rights)
        {
            for (size_t i = 0; i < right.size(); ++i)
                if (right[i] == symbol)
                {
                    if (i + 1 < right.size())
                    {
                        Symbol nextSymbol = right[i + 1];
                        if (find(terminals.begin(), terminals.end(), nextSymbol) != terminals.end())
                        {
                            if (follow[symbol].insert(nextSymbol).second)
                                changed = true;
                        }
                        else
                        {
                            for (const auto &firstSym : first.at(nextSymbol))
                            {
                                if (firstSym != "ε")
                                {
                                    if (follow[symbol].insert(firstSym).second)
                                        changed = true;
                                }
                            }
                            if (first.at(nextSymbol).count("ε"))
                            {
                                compute_FOLLOW(rule.left, visited, changed);
                                for (const auto &followSym : follow[rule.left])
                                {
                                    if (follow[symbol].insert(followSym).second)
                                        changed = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        compute_FOLLOW(rule.left, visited, changed);
                        for (const auto &followSym : follow[rule.left])
                        {
                            if (follow[symbol].insert(followSym).second)
                                changed = true;
                        }
                    }
                }
        }
    }

    visited.erase(symbol);
}

FOLLOW Grammar::get_FOLLOW(bool display = false)
{
    if (!follow.empty())
    {
        if (display)
            displaySet(follow);
        return follow;
    }
    if (first.empty())
        get_FIRST();

    set<Symbol> visited;

    // 初始化所有非终结符的FOLLOW集为空
    for (const auto &nonterminal : nonterminals)
        follow[nonterminal] = set<Symbol>();

    // 起始符号的FOLLOW集包含终止符号$
    if (!nonterminals.empty())
        follow[nonterminals[0]].insert("$");

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto &nonterminal : nonterminals)
            compute_FOLLOW(nonterminal, visited, changed);
    }

    if (display)
        displaySet(follow);
    return follow;
}

// 根据非终结符的FIRST集计算FIRST集
set<Symbol> Grammar::calf(vector<Symbol> seq)
{
    get_FIRST();
    set<Symbol> firstSet;
    for (const auto &symbol : seq)
    {
        if (find(terminals.begin(), terminals.end(), symbol) != terminals.end())
        {
            firstSet.insert(symbol);
            break;
        }
        else
        {
            for (const auto &firstSym : first[symbol])
            {
                if (firstSym != "ε")
                    firstSet.insert(firstSym);
            }
            if (first[symbol].count("ε") == 0)
                break;
        }
    }
    return firstSet;
}

// 判断是否是LL(1)文法
bool Grammar::isLL1()
{
    get_FIRST();
    get_FOLLOW();

    for (auto &nt : nonterminals)
    {
        auto it = find(rules.begin(), rules.end(), Rule{nt, {}});
        if (it == rules.end())
            continue;
        set<Symbol> convergance;
        for (auto &right : it->rights)
        {
            set<Symbol> tmp = calf(right);
            set<Symbol> intersection;
            set_intersection(convergance.begin(), convergance.end(), tmp.begin(), tmp.end(), inserter(intersection, intersection.begin()));
            if (intersection.size() != 0)
                return false;
            set_union(convergance.begin(), convergance.end(), tmp.begin(), tmp.end(), inserter(convergance, convergance.begin()));
        }
    }
    return true;
}

bool Grammar::generateParsingTable(bool display = false)
{
    if (isLL1() == false)
    {
        cout << "不是LL(1)文法" << endl;
        return false;
    }
    if (!pt.empty())
        return true;

    for (const auto &rule : rules)
    {
        Symbol left = rule.left;
        for (const auto &right : rule.rights)
        {
            set<Symbol> firstSet = calf(right);
            for (const auto &terminal : firstSet)
            {
                if (terminal != "ε")
                    pt[left][terminal] = right;
            }
            if (firstSet.find("ε") != firstSet.end())
            {
                for (const auto &terminal : follow[left])
                    pt[left][terminal] = right;
            }
        }
    }
    if (display)
    {
        cout << "预测分析表:" << endl;
        cout << setw(10) << " ";
        for (const auto &terminal : terminals)
        {
            if (terminal == "ε")
                continue;
            else
                cout << setw(10) << terminal;
        }
        cout << endl;

        for (const auto &nonterminal : nonterminals)
        {
            cout << setw(10) << nonterminal;
            for (const auto &terminal : terminals)
            {
                if (terminal == "ε")
                    continue;
                else if (pt[nonterminal].find(terminal) != pt[nonterminal].end())
                {
                    string production;
                    production += nonterminal + " -> ";
                    for (const auto &sym : pt[nonterminal][terminal])
                        production += sym + " ";
                    cout << setw(10) << production;
                }
                else
                    cout << setw(10) << " ";
            }
            cout << endl;
        }
    }
    return true;
}

bool Grammar::LL1_parser(const vector<Symbol> &input)
{
    if (!generateParsingTable())
    {
        cout << "预测分析表生成失败" << endl;
        return false;
    }
    stack<Symbol> s;
    s.push(nonterminals[0]);
    vector<Symbol> tmp = input;
    tmp.push_back("$");

    cout << "输入串: ";
    for (const auto &sym : input)
        cout << sym << " ";
    cout << endl;

    while (!s.empty())
    {
        cout << "栈内元素:";
        stack<Symbol> tmp_s = s;
        while (!tmp_s.empty())
        {
            cout << tmp_s.top() << " ";
            tmp_s.pop();
        }

        Symbol top = s.top();
        s.pop();
        Symbol current_symbol = tmp[0];

        cout << endl;
        cout << "输入串:";
        for (const auto &sym : tmp)
            cout << sym << " ";
        cout << endl;

        cout << endl;

        if (find(terminals.begin(), terminals.end(), top) != terminals.end())
        {
            if (top == current_symbol)
            {
                cout << "匹配终结符: " << top << endl;
                tmp.erase(tmp.begin());
            }
            else
            {
                cout << "错误: 终结符不匹配" << endl;
                return false;
            }
        }
        else
        {
            if (pt[top].find(current_symbol) == pt[top].end())
            {
                cout << "错误: 无法从预测分析表中找到对应的产生式" << endl;
                return false;
            }
            else
            {
                vector<Symbol> right = pt[top][current_symbol];
                cout << "使用产生式: " << top << " -> ";
                for (const auto &sym : right)
                    cout << sym << " ";
                cout << endl;

                for (auto it = right.rbegin(); it != right.rend(); ++it)
                    if (*it != "ε")
                        s.push(*it);
            }
        }
    }
    if (tmp[0] == "$")
    {
        cout << "成功: 输入串被成功解析" << endl;
        return true;
    }
    cout << "错误: 输入串未能完全解析" << endl;
    return false;
}
