#ifndef LL1_H
#define LL1_H

#include "colors.h"
#include <stack>
#include <vector>
#include <list>
#include <string>
#include <fstream> //for ifstream
#include <mutex>
#include <queue>
#include <iostream>
using namespace std;

class LL1
{
private:
    Colors COLOR;
    map<string,vector<vector<string>>> productions; //productions
    list<string> productionKeys; //to order by insert
    list<string> colKeys; //columns in order by insert
    map<string,int> t; //non terminals
    map<string,map<string,int>> table; //end table
    map<string,list<string>> first; //first set
    map<string,list<string>> follow; //follow set
    map<string,list<string>> firstp;  //first+ set
    string deliminator; //what we are using to deliminate A from B
    string epsilon; // the symbol used for epsilon
    string start; //the start symbol
public:
    /**
     * This allows for a string filename for construction of the LL1
     * 
     * @param file string filename
     * @param delim the deliminator used to deliminate A from productions, defaults to " -> "
     * @param epsilon the symbol used for epsilon, defaults to "~"
     */
    LL1(string& file, string delim = " -> ", string epsilon = "~") : LL1(ifstream(file), delim, epsilon) {}
    /**
     * takes a file and processes productions
     * 
     * @param file ifstream file to open
     * @param delim what is used to deliminate A from B
     * @param epsilon what we are using for the epsilon symbol
     * 
     * @throws runtime_error if file can't be opened
     */
    LL1(ifstream file, string delim = " -> ", string epsilon = "~")
    {
        if (!file.is_open()) //error checking
        {
            throw runtime_error(COLOR.printInColor("Red", "Error loading file."));
        }

        vector<string> lines;
        string line;
        while(getline(file,line))
        {
            lines.push_back(line);
        }

        init(lines, delim, epsilon);
    }
    /**
     * takes a string[] and processes productions
     * 
     * @param s string array to be processed
     * @param delim what is used to deliminate A from B
     * @param epsilon what we are using for the epsilon symbol
     */
    template <size_t N>
    LL1(string (&s)[N], string delim = " -> ", string epsilon = "~")
    {
        vector<string> p(begin(s), end(s));
        init(p, delim, epsilon);
    }
    /**
     * takes a vector<string> and processes productions
     * 
     * @param s vector of strings to be processed
     * @param delim what is used to deliminate A from B
     * @param epsilon what we are using for the epsilon symbol
     */
    LL1(vector<string> &s, string delim = " -> ", string epsilon = "~")
    {
        init(s, delim, epsilon);
    }
    /**
     * Prints the First set
     */
    void printFirst()
    {
        PrintSet("First", this->first);
    }
    /**
     * Prints the Follow set
     */
    void printFollow()
    {
        PrintSet("Follow", this->follow);
    }
    /**
     * Prints the First+ set
     */
    void printFirstPlus()
    {
        PrintSet("First+", this->firstp);
    }
    /**
     * Prints productions with corresponding index
     */
    void printProductions()
    {
        printf("Productions:\n");
        int j = 0;
        for(auto& key : productionKeys)
        {
            string k = key; //if there is more than one production per key, this changes
            string d = deliminator; //I use a different deliminator for multiple productions for the same A
            for(auto& b : productions[key])
            {
                //i.e 0   Goal -> Expr
                printf("%d\t%s\t%s\t",j ,k.c_str(),d.c_str());
                for(auto& bn : b)
                {
                    printf("%s ", bn.c_str());
                }
                printf("\n");
                k = string(key.size(), ' '); //set key to blank spaces the size of A's key, for alignment
                d = " | "; //deliminator is now this
                j++; //iterate rule #
                //if there are more productions for a given A, it will use the new k and d values.
            }
        }
        printf("\n\n");
    }
    /**
     * prints the table and productions to visualize table
     */
    void printTable()
    {
        printf("Table:\n");
        //print columns in insert order
        for(auto& col : colKeys)
        {
            if(col == epsilon)
            {
                continue;
            }
            printf("\t%s", col.c_str());
        }
        printf("\n");

        //go through productions in insert order and print the rule #
        int i = 0;
        for(auto& kv : productionKeys)
        {
            printf("%s\t",kv.c_str());
            int max = -1; //I have to convert the rule # so that matches the text
            for(auto& col : colKeys)
            {
                if(col == epsilon) //don't print epsilon column
                {
                    continue;
                }
                if(table[kv][col] == -1) //convert -1 to a dash
                {
                    printf("-\t");
                }
                else
                {
                    printf("%d\t",table[kv][col] + i); //print the rule number plus offset i
                }
                max = max > table[kv][col] ? max : table[kv][col]; //get the largest number
            }
            printf("\n");
            /**
             * in parsing, I have a map where the key is the main production, and the value is a
             * vector of productions. So instead of going to rule 3, I go to production[Expr'][1].
             * But I wanted to make sure the logic would be the same, so I calculate what the
             * rule number would be if I did this in a not insane way. That is what i and max are for.
             */
            i += max+1;
        }
        
        printf("\n\n");
    }
    /**
     * Parses a given line from tokenizer.h -> tokens
     * 
     * @param words a vector of tokens where first is the token type and value is the token
     * @returns true if valid, false if not valid.
     */
    void parseLine(vector<pair<string,string>> words)
    {
        if(words.empty())
            return;
        //push eof into the tokens, so that the algorithm knows when to quit
        words.push_back(pair<string,string>("eof","eof"));
        pair<string,string> word = words[0]; //word <- next word
        int index = 1; // this is so we can get the next word in the algorithm
        stack<string> s; //stack
        s.push("eof"); //push eof onto stack
        s.push(this->start); //push start symbol onto stack
        string focus = s.top(); //focus <- stack top
        while(true) //"infinite" loop
        {
            string w = word.first == "name" || word.first == "num" ? word.first : word.second;
            if(w == "")
            {
                continue;
            }
            w = word.first == "int32" || word.first == "float32" ? "num" : w;
            //If we got to the end of the tokens and stack
            if(focus == "eof" && w == "eof")
            {
                //this is a valid expression
                return;
            }
            //if focus is a terminal or focus is eof
            
            else if (isTerminal(focus) || focus == "eof")
            {
                if(focus == w) //if focus is word
                {
                    s.pop(); //pop the stack
                    word = words[index]; //word <- next word
                    index++; //for the next word
                    while(word.first == "")
                    {
                        word = words[index];
                        index++;
                    }
                }
                else //something went wrong
                {
                    string exprErr = "";
                    int exprSize = 0; //we check if we are at the index of the error to print the symbol in red
                    int carrotPrint = 0;
                    bool countCarrot = true;
                    //we are going to print the expression
                    for(auto &kv : words)
                    {
                        //since I am adding spaces between the tokens, (index-1)*2 will set the position to the
                        //correct spot, based on the index of the errored symbol
                        if(exprSize == (index-1)*2)
                        {
                            //print the error symbol in red
                            exprErr += COLOR.printInColor("Red", kv.second + " ");
                            //change exprSize so the next iteration it won't be equal
                            exprSize += 2;
                            carrotPrint += (kv.second.size()/2) * countCarrot;
                            countCarrot = false;
                            continue;
                        }
                        //make sure we don't print eof, unless there is a missing ")", which will print above
                        if(kv.second != "eof")
                        {
                            //print the token
                            exprErr += COLOR.printInColor("White", kv.second + " ");
                            //change expression size
                            exprSize += 2;
                        }
                        carrotPrint += (kv.second.size() + 1) * countCarrot;
                    }
                    //printf("\n");
                    //print the ^ in red under the errored symbol
                    //cerr << string((index-1)*2, ' ') << COLOR.printInColor("Red", "^") << "\n";
                    string carrot = string(carrotPrint,' ') + COLOR.printInColor("Red","^");
                    throw runtime_error("invalid symbol or missing parenthesis\n" + exprErr + "\n" + carrot);
                }
            }
            else //for non terminals
            {
                //if there is a production rule
                if(table[focus][w] != -1)
                {
                    s.pop(); //pop the stack
                    //get the production <- productions[key][index]
                    vector<string> production = productions[focus][table[focus][w]];
                    for(int i = production.size()-1; i >= 0; i--)
                    {
                        //go backwords and push the productions onto the stack, except epsilon
                        if(production[i] != epsilon)
                        {
                            s.push(production[i]);
                        }
                    }
                }
                //no production rule
                else
                {
                    string exprErr = "";
                    //The rest is the same as above, find the symbol, print it in red, then add a ^ under it
                    int exprSize = 0;
                    int carrotPrint = 0;
                    bool countCarrot = true;
                    for(auto &kv : words)
                    {
                        if(exprSize == (index-1)*2)
                        {
                            //print the error symbol in red
                            exprErr += COLOR.printInColor("Red", kv.second + " ");
                            //change exprSize so the next iteration it won't be equal
                            exprSize += 2;
                            carrotPrint += (kv.second.size()/2) * countCarrot;
                            countCarrot = false;
                            continue;
                        }
                        //make sure we don't print eof, unless there is a missing ")", which will print above
                        if(kv.second != "eof")
                        {
                            //print the token
                            exprErr += COLOR.printInColor("White", kv.second + " ");
                            //change expression size
                            exprSize += 2;
                        }
                        carrotPrint += (kv.second.size()+1) * countCarrot;
                    }
                    //printf("\n");
                    string carrot = string(carrotPrint,' ') + COLOR.printInColor("Red","^");
                    throw runtime_error("error processing symbol: \n" + exprErr + "\n" + carrot);
                }
            }
            focus = s.top(); //focus <- top
        }
    }
private:
/**
     * actual initializer for multiple constructors
     * 
     * @param s vector of strings that are the productions
     * @param delim the deliminator used between A and B1,B2...Bn, defaults to " -> " as in "A -> B1 B2 ... Bn"
     * @param epsilon the symbol used for epsilon, defaults to "~"
     */
    void init(vector<string> &s, string delim = " -> ", string epsilon = "~")
    {
        this->deliminator = delim; //set deliminator
        this->epsilon = epsilon; //set epsilon
        map<string,int> columns{{"eof",-1}}; //these will be the columns
        colKeys.push_back("eof"); //this is to sort by insert order
        for(string& prod : s) //for each production
        {
            string lhand = prod.substr(0, prod.find(deliminator)); //get the left hand
            string rhand = prod.substr(prod.find(deliminator) + deliminator.size(), prod.size()); //get the right hand
            rhand = rhand + " "; //add a space so that I can split the string and not lose the last item
            productions[lhand].push_back(vector<string>()); // create a new production A
            productionKeys.push_back(lhand); // for sorting in insert order
            //split the right hand side into it's individual tokens
            while(rhand.find(" ") != -1) 
            {
                string token = rhand.substr(0,rhand.find(" "));
                if(!isalpha(token.at(0)) || !isupper(token.at(0)))
                {
                    if(columns.find(token) == columns.end())
                        colKeys.push_back(token);
                    columns[token] = -1;
                }
                productions[lhand].back().push_back(token); //add bn to A
                rhand = rhand.substr(rhand.find(" ")+1, rhand.size());
            }
        }
        //I initialize the table here instead of later
        for(auto& kv : productions)
        {
            table[kv.first] = columns;
        }
        t = columns; //globalize the columns for use later
        productionKeys.unique(); //get rid of duplicates
        buildFirstSet(); //build First
        buildFollowSet(); //build Follow
        buildFirstPlus(); //build First+
        buildTable(); //build Table
        return;
    }
    /**
     * Prints the First, Follow, or First+ sets
     * 
     * @param name string name of the set
     * @param A The set to print, map<string,list<string>> where the key is the column and the value is a list of T's and TN's
     */
    void PrintSet(string name, map<string,list<string>> &A)
    {
        /**
         * The goal is to take my set structure, print the production names on top
         * and then all the operators below it
         */
        printf("%s:\n",name.c_str()); //print the name of the set
        vector<list<string>> printArr; //this will be the actual thing that gets printed
        printArr.push_back({}); //initialize the first list
        int k = 0; // We need to keep track of how many columns we have done, so we can add spaces
        for(auto& kv : A) //for each operator in production
        {
            printArr.at(0).push_back(kv.first); //the top list will be the column names
            int i = 1; //rows 1->n are the values of the list
            for(auto &t : kv.second) //for each operator in the list
            {
                if(t.empty()) //don't do "" symbol
                {
                    continue;
                }
                //if we haven't made a list for this depth yet
                if(i >= printArr.size())
                {
                    //push a new list into the spot
                    printArr.push_back({});
                    for(int j = 1; j <= k; j++)
                    {
                        //we want to put the operator under the column, so we have to buffer with spaces
                        printArr.at(i).push_back(" "); 
                    }
                    printArr.at(i).push_back(t);
                }
                else
                {
                    int s = k - printArr.at(i).size();
                    //This loop offsets the horizontal position so that the operator falls under the correct column
                    for(int j = 0; j < s; j++)
                    {
                        printArr.at(i).push_back(" "); 
                    }
                    printArr.at(i).push_back(t); //finally push the operator into the spot
                }
                i++;
            }
            k++;
        }
        //finally, print everything
        for(auto& line : printArr)
        {
            for(auto &w : line)
            {
                printf("%s\t", w.c_str());
            }
            printf("\n");
        }
        printf("\n");
        return;
    }
    
    /**
     * merges two tables without emptying the merged array
     * 
     * @param A list<string> address, for in-place merging
     * @param B the list to merge into A without changing the original list
     */
    void mergeSets(list<string>& A, list<string> B)
    {
        for(auto& b : B)
        {
            A.remove(b); //make sure we aren't duplicating, without sorting
        }
        B.sort(); //Linux is fine without this, but will crash on Windows without it.
        A.merge(B);
    }
    /**
     * determines whether a token is a terminal
     * 
     * @param key token to check
     * @return true if terminal, false if not
     */
    bool isTerminal(string key)
    {
        return productions.find(key) == productions.end();
    }
    /**
     * Builds the First Set
     */
    void buildFirstSet()
    {
        //initialize First Set with NTs
        for(auto& kv : productions)
        {
            first[kv.first].push_back({});
        }
        //add in Ts
        for(auto& kv : t)
        {
            first[kv.first].push_back(kv.first);
        }
        bool hasChanged = true; //has First changed?
        while(hasChanged)
        {
            hasChanged = false;
            for(auto& p : productions) // NT -> Production
            {
                for(auto& b : p.second) //Production -> b1,b2,b3...bk
                {
                    int k = b.size();
                    int i = 0;
                    list<string> rhs = first[b[0]]; // rhs <- first(b1)
                    rhs.remove({}); // - {ε}
                    while(first[b[i]].front() == epsilon && i < k-1) //if epsilon and i < k
                    {
                        list<string> bn = first[b[i+1]];
                        bn.remove({});
                        mergeSets(rhs,bn); // rhs <- rhs U first(bn - ε)
                        i++;
                    }
                    //if i = k and epsilon
                    if(i == k-1 && first[b[k-1]].front() == epsilon)
                    {
                        rhs.push_back({});
                    }
                    list<string> org = first[p.first]; //make an original
                    mergeSets(first[p.first], rhs); //merge the two sets
                    hasChanged = hasChanged || first[p.first] != org; //check if there was a change
                }
            }
        }
    }
    /**
     * builds the Follow Set
     */
    void buildFollowSet()
    {
        bool startFound = false;
        for(auto& kv : this->productions) // for each A Σ NT
        {
            // Start production will start with a *
            if(!startFound && kv.first.at(0) == '*') 
            {
                this->follow[kv.first].push_back("eof"); // Follow(S) <- {eof}
                this->start = kv.first; //set the start symbol
                startFound = true;
                continue;
            }
            this->follow[kv.first].push_back({}); //Follow(A) <- Θ
        }

        bool hasChanged = true;
        //while (follow sets are still changing)
        while(hasChanged)
        {
            hasChanged = false;
            for(auto& p : this->productionKeys)
            {
                list<string> trailer = this->follow[p]; //trailer <- follow[A]

                for(auto& b : productions[p]) //for each production
                {
                    int k = b.size()-1; //get the number of b's in production
                    for(int i = k; i >= 0; i--) //count down from last to first
                    {
                        if(!isTerminal(b[i])) //if bi is NT
                        {
                            list<string> org = this->follow[b[i]]; //create original
                            mergeSets(this->follow[b[i]],trailer); //merge sets
                            hasChanged = this->follow[b[i]] != org; //check if something changed

                            //check for epsilon
                            bool hasEpsilon = false;
                            for(auto& s : this->first[b[i]])
                            {
                                hasEpsilon = s == epsilon;
                                if(hasEpsilon)
                                {
                                    break;
                                }
                            }
                            //if there is an epsilon
                            if(hasEpsilon)
                            {
                                list<string> U = first[b[i]];
                                U.remove(epsilon);
                                mergeSets(trailer, U); //trailer <- trailer U (first(bi) - ε)
                            }
                            else
                            {
                                trailer = first[b[i]]; //trailer <- first(bi)
                            }
                        }
                        else
                        {
                            trailer = first[b[i]]; //trailer <- first(bi)
                        }
                    }
                }
            }
        }
    }
    /**
     * build first plus
     */
    void buildFirstPlus()
    {
        for(auto& p : this->first) //for each element in first
        {
            //check for epsilon
            bool hasEpsilon = false;
            for(auto& s : p.second)
            {
                hasEpsilon = s == epsilon;
                if(hasEpsilon)
                {
                    break;
                }
            }
            firstp[p.first] = p.second; //firstp[bn] <- first(bn)
            if(hasEpsilon) //if there is an epsilon
            {
                mergeSets(firstp[p.first], follow[p.first]); //merge follow into that
            }
        }
    }

    /**
     * build the table
     */
    void buildTable()
    {
        /**
         * Because I decided to use maps, this was a bit tricky.
         * Essentially, in the parser section, when checking a token
         * I will use the row key A, and use the index for production
         * i.e. [Term'][+] is 8 in example, it will be 2 in my algortithm
         * and it reads "go to Term', production 2". Since Term' is the 6th production,
         * plus 2 is 8.
         */
        for(auto& key : productionKeys)
        {
            int rule = 0;
            for(auto& b : productions[key]) //go through each production
            {
                for(auto& w : firstp[b[0]]) //take b1 from b and get firstplus
                {
                    if(w == "") //don't log these
                    {
                        continue;
                    }
                    if(w == epsilon) //we need to add the follow stuff
                    {
                        for(auto& t : firstp[key]) //go to firstp[A]
                        {
                            if(t == epsilon || t == "") //don't log these
                            {
                                continue;
                            }
                            table[key][t] = table[key][t] == -1 ? rule : table[key][t]; //if there is no rule, set the rule
                        }
                        continue;
                    }
                    table[key][w] = table[key][w] == -1 ? rule : table[key][w]; //set the rule
                }
                rule++; //increment to the next rule
            }
        }
    }
};

#endif