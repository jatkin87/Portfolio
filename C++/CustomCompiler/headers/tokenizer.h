#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <regex>
#include <iostream>
#include <map>
using namespace std;

static bool PRINT_CATS = false;
/*
 *  Checks a given expression against regex's to determine it's token
 */
class Tokenizer
{
private:
    //where I will store the individual tokens for this expression
    map<int,pair<string,string>> tokens{};
    vector<string> order = {"comment", "nasm", "if", "type", "name", "float32", "int32", "operator", "ternary","bracket"};
    //the regular expressions to check against
    map<string,regex> regex_expressions
    {
        {"comment", regex("(\\/\\/(.)*)$")}, //comments, want to check first
        {"nasm", regex("(BYTE)|([A-Z]*WORD)\\[[a-zA-Z\\-0-9]*\\]")},
        {"if", regex("(if|IF)\\s*\\(.*\\)\\s*\\{.*\\}((else|ELSE)\\s*\\{.*\\})*")},
        {"ternary", regex("(<=)|(>=)|([\\?\\:<>]|(==)|(\\!=)){1}")}, //ternary if statements
        {"bracket", regex("[(){}<>\"\']")}, //brackets
        {"type", regex("([a-zA-Z_]+[a-zA-Z0-9_]*)(?=\\s+[a-zA-Z_]+[a-zA-Z0-9_]*\\s*=)")},
        {"float32", regex("(\\d+\\.\\d+)|(\\.\\d+)")}, //float
        {"int32", regex("\\b\\d+")}, //int
        {"name", regex("[a-zA-Z_]+[a-zA-Z0-9_]*")}, //name
        {"operator", regex("([\\*\\+\\-\\/\\^,;])|([^=]|^)=([^=]|$)|(<<)|(?!\\.\\d+)\\.")}, //operators
        {"line", regex("[^;]*;")}
        
    };
public:
    /**
     * Default constructor for initialization purposes
     */
    Tokenizer() {};
    /**
     * Constructor. Takes a string and passes to tokenizer method
     * 
     * @param string& expression to tokenize
     */
    Tokenizer(string& s, vector<string> *ord = nullptr)
    {
        TokenizeString(s, ord);
    }
    /**
     * get tokens
     * 
     * @return a vector of string pairs where first is the token category and second is the token
     */
    vector<pair<string,string>> getTokens()
    {
        vector<pair<string,string>> t;
        for(auto &kv : tokens)
        {
            if(kv.second.first == "comment")
            {
                continue;
            }
            t.push_back(kv.second);
        }
        return t;
    }
    /**
     * Print the tokens found in this expression
     */
    void PrintTokens()
    {
        cout << '"';
        int i = 1;
        for(auto& kv : tokens)
        {
            //My super annoying way of determining if we print a comma without using a boolean
            int num_commas = (((int)tokens.size()-1-i >> (sizeof((int)tokens.size()-1-i)*8)-1)+1);
            /**
             * 
             */
            string item = PRINT_CATS ? kv.second.first : kv.second.second;
            cout << " " << item << " " << string(num_commas,','); //print the commas from the above equation
            i++;
        }
        cout << '"';
    }
    /**
     * This is used if default constructor was used, allowing for multiple evaluations through one object
     */
    void Expr(string& s)
    {
        //If there are tokens from the previous expression, remove them.
        for(auto kv : tokens)
        {
            tokens.erase(kv.first);
        }
        TokenizeString(s);
    }
private:
    /**
    * Takes a string and turns it into tokens
    * 
    * @param string& check_string string to tokenize
    */
    void TokenizeString(string& check_string, vector<string> *ord = nullptr)
    {
        if(!ord)
        {
            ord = &order;
        }
        string s = check_string;
        smatch string_match;
        for(auto& key : *ord) //for each regular expression
        {
            auto exp = pair<string,regex>({key, regex_expressions[key]});
            string text = s; //make a copy of the string
            int offset = 0; //so we can calculate proper position in the string
            while(regex_search(text,string_match,exp.second))
            {
                //I am going to remove this match from the main string, it helps prevent a regex picking up another category
                s.replace(string_match.position(0)+offset,string_match.str(0).size(),string_match.str(0).size(),' ');
                //if(DEBUG) {cout << s << "\t**remove " << exp.first << "**" << '\n';}
                //catch the match, remove spaces
                string match = regex_replace(string_match.str(0),regex("\r"),"");
                match = regex_replace(string_match.str(0),regex(" "),"");
                int match_index = string_match.position(0) + offset; //get the index where it exists in the string
                tokens[match_index] = pair<string,string>(exp.first,match); //put it in, but put category in too
                text = string_match.suffix().str(); //get the rest of the text
                offset = s.length() - text.length(); //calculate the offset for how much was removed
            }
        }
    }
public:
    static string formatIf(string& expression)
    {
        vector<string> ord {"type","int32","float32", "name", "operator", "ternary", "bracket"};
        Tokenizer t(expression, &ord);
        string expr = "";
        bool boolFound = false;
        bool bFound = false;
        bool pFound = false;
        bool ifBody = true;
        int pCount = 0;
        int bCount = 0;
        for(auto& token : t.getTokens())
        {
            pFound = pFound || token.second == "(";
            pCount += (token.second == "(") - (token.second ==")");
            if(pFound && !boolFound && pCount == 0)
            {
                expr += "?(";
                boolFound = true;
                continue;
            }
            bFound = bFound || token.second == "{";
            bCount += (token.second == "{") - (token.second == "}");
            if(bFound && bCount == 0)
            {
                bFound = false;
                expr += ifBody ? "):(" : ")";
                ifBody = false;
            }
            expr += token.second == "(" || 
                    token.second == ")" || 
                    token.second == "{" || 
                    token.second == "}" || 
                    token.second == "if" || 
                    token.second == "else" ? "" : token.second + " ";
        }
        return expr[expr.size()-1] == '(' ? expr + "void)" : expr;
    }

    static vector<pair<string,string>> getLines(string expression)
    {
        vector<string> ord = {"line"};
        Tokenizer t(expression, &ord);
        return t.getTokens();
    }
};

#endif