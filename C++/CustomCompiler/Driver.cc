#include "./headers/LL1.h"
#include "./headers/tokenizer.h"
#include "./headers/symboltable.h"
#include "./headers/nasmconverter.h"
#include "./headers/optimizer.h"

using namespace std;

Colors COLOR; //for pretty printing

/**
 * the thing that drives everything
 */
class Driver
{
private:
    string currScope = "main:"; //I prepped for functions and scopes, but I got outvoted
    vector<string> scopeKeys {"function_", "scope_", "main"}; //the only thing that will run atm is main
    map<string, int> scopeCounts {{"function_", 0}, {"scope_", 0}, {"main", 1}}; //I was going to use this to count functions for scope labels in NASM
    map<string, SymbolTable *> scope {{"main:", new SymbolTable()}}; //our symbol table object
    LL1 * ll; //our productions
    NASMConverter * nasm; //our nasm converter object
public:
    ~Driver()
    {
        //clean our stuff up
        for(auto &kv : scope)
        {
            delete scope[kv.first];
            scope[kv.first] = nullptr;
        }
        delete nasm;
        nasm = nullptr;
        delete ll;
        ll = nullptr;
    }
    /**
     * constructor also starts the process
     * 
     * @param f the file to convert
     * @param optimize if we want to optimize the code before generating nasm
     */
    Driver(ifstream f, string output = "output", bool optimize = false)
    {
        //obviously error if the file can't open
        if(!f.is_open())
        {
            throw runtime_error("Error loading file.");
        }
        this->nasm = new NASMConverter(output); //for writing the nasm file
        this->ll = new LL1(ifstream("productions.txt")," -> ", "ε"); //load our productions
        string expr; //line
        int line = 0; //current line number
        /**
         * ok, so I overthought this and I turn all if statements into ternary statements.
         * I figured that a ternary statement can be stored in a tree so it should be easier.
         * Really, I just needed to add signals to the symbol table to determine when if|else start and end
         * and I could have printed just fine. Instead, I accept ternaries and I do this stuff to convert
         * the if statement into a ternary. It works, but it was a struggle
         */
        bool isIf = false; //if we are in an if statement
        bool useIf = false; //if the if statement has all been read and needs to be processed
        bool inBody = false; //if we are in an if body
        int afterBracket = 0; //this is used to determine when a body starts and ends
        string ifstm = ""; //I fill this up with the if statement to be converted into a ternary statement
        //because I have to process the if statement, I need to capture the line that would be processed after the if statement
        queue<string> exprMng{}; //so I have a queue where I will insert the lines
        while(getline(f,expr) || !exprMng.empty() || useIf)
        {
            if(expr == "\r") //I'm on windows, so these don't get removed in my tokenizer
                continue;
            int rpos = expr.find('\r');
            if(rpos > -1)
                expr = expr.replace(rpos, 1, ""); //remove the /r at the end of the string
            if(!expr.empty()) //if we have a line
                    exprMng.push(expr); //put the line in
            try
            {
                //I wrote this stuff in here, but then at some point I needed to call this same code
                //so I made it into a function. All inputs are modified by the function
                processExpr(ifstm, line, afterBracket, isIf, inBody, useIf, optimize, exprMng);
                expr = ""; //when we have no more lines to read, this will retain its last 
            }
            catch(const std::exception& e)
            {
                useIf = false; //should be false anyway, but if an if statement is bad we don't want the next expression to process the ifstatement
                expr = "";
                cout << COLOR.printInColor("Red", "Line " + to_string(line) + ": " + e.what()) << endl;
            }
        }
        writeToNasm(optimize); //finally write the nasm file
    }
    /**
     * processes just one expression. For if statements
     * 
     * @param expr address to the expression string
     * @param optimize are we optimizing?
     * @param line line number
     */
    void processExpr(string& expr, bool optimize, int line)
    {
        bool isIf = false;
        bool useIf = false;
        bool inBody = false;
        int afterBracket = 0;
        string ifstm = "";
        queue<string> exprMng{};
        exprMng.push(expr);
        processExpr(ifstm, line, afterBracket, isIf, inBody, useIf, optimize, exprMng, false);
    }
    /**
     * processes an expression from string to tree, from tree to nasm
     * 
     * @param ifstm this is used to store multi-lined if statements before processing
     * @param line line number
     * @param afterBracket int to count brackets for if bodies
     * @param isIf if wer are processing an if statement
     * @param inBody if we are in an if body
     * @param useIf if we have gotten the full if statement into one line and need to process it
     * @param optimize are we optimizing the expressions before converting to nasm?
     * @param exprMng holds the expression we are processing
     * @param useLL if we are calling this function later on, we have already checked the LL so no need to again
     */
    void processExpr(string& ifstm, int& line, int& afterBracket, bool& isIf, bool& inBody, bool& useIf, bool& optimize, queue<string>& exprMng, bool useLL = true)
    {
        //for storing the c-line for comments in the NASM file
        string expr = "";
        afterBracket = afterBracket * isIf; //if isIf is false, this resets afterBracket to 0
        //this boolean determines if we are in an if|else statement
        isIf = (isIf && !exprMng.empty()) || exprMng.front().size() > 1 && (exprMng.front().substr(0,2) == "if" || exprMng.front().substr(0,4) == "else");
        //if we are in an if statement, and we have an expression to process
        if(isIf && !exprMng.empty())
        {
            //this determines whether we are in a body
            inBody = (inBody && exprMng.front()[exprMng.front().size()-1] != '}') || exprMng.front()[exprMng.front().size()-1] == '{';
            //if not in body, resets to 0, else increments by 1
            afterBracket = ((afterBracket * inBody) + (1 * inBody));
            //if there isn't an else, resets to 0. Stays the same if there is an else
            afterBracket = afterBracket * !(exprMng.front()[0] == '}' && exprMng.front()[exprMng.front().size()-1] == '{');
            //if there is an else, we will increment by 1
            afterBracket += (exprMng.front()[0] == '}' && exprMng.front()[exprMng.front().size()-1] == '{');
            //check if we are still in an if statement
            isIf = exprMng.front()[exprMng.front().size()-1] != '}'; 
            //if we aren't, we will use this statement for the next process
            useIf = !isIf;
            //add the line to the if statement
            //if we are in a body, append a ; symbol so the tree knows these are separate lines
            ifstm += exprMng.front() + (inBody && afterBracket > 1 ? "; " : " ");
            //pop the last expression
            exprMng.pop();
            //any time we pop, we want to increment the line number by 1
            line += 1;
            return;
        }
        //if we have an if statement, use that
        string expression = useIf ? ifstm : exprMng.front();
        if(expression.empty())
        {
            return;
        }
        //create a new optimizer, using LL if necessary
        Optimizer* o = useLL ? new Optimizer(expression, ll) : new Optimizer(expression);
        //if we are here and don't have an if statement
        if(!useIf)
        {
            expr = exprMng.front(); //get the expression
            exprMng.pop(); //pop it out
            line += 1; //incr line number
        }
        else
        {
            useIf = false; //reset this stuff
            ifstm = "";
        }
        if(!o->root) //get rid of empty lines
        {
            exprMng.pop();
            return;
        }
        if(o->root->getValue().second == "<<") // if we have a function call
        {
            o->process(scope[currScope]); //process with symbol table
            //there might be multiple function calls, each needs a unique key
            string key = to_string(line) + "_" + o->root->getLeft()->nasmExpr;
            scope[currScope]->insertOrder.push_back(key); //put the key into the symbol table for insert order
            string value = o->root->nasmExpr; //the nasm expression is both how I store into the symbol table, but also helps with printing NASM
            scope[currScope]->addValue(key, value, line, "function"); // add this key into the symbol table
            int dindex = scope[currScope]->st[o->root->getRight()->getValue().second].depIndex;
            string dep = scope[currScope]->st[o->root->getRight()->getValue().second].dependsOn[dindex]; //this is the parameter to print
            dep = scope[currScope]->st[dep].type; //get the type
            scope[currScope]->st[key].dependsOn.push_back(dep); //save the type
            for(auto &x : scope[currScope]->st)
            {
                //if a variable gets optimized out, but then we have a print,
                //we need to make sure everything above the print processes so that the print works right
                x.second.dep = 99;
            }
            //comment of the expression for NASM
            scope[currScope]->cLine[key] = "; Line " + to_string(line) + ": " + expr + "\n";
            return;
        }
        //ternary
        if(o->root->getValue().second == "?")
        {
            o->root->optimizePar(); //get rid of parentheses
            o->root->getLeft()->process(scope[currScope]);
            string key = "if?_" + to_string(line); //the if label is if?_line#. if printing on line 5, the label will be if?_5:
            scope[currScope]->insertOrder.push_back(key); //put the key into the insert order
            scope[currScope]->addValue(key, o->root->getLeft()->nasmExpr, line, "if"); //add this in as an "if" type

            //the ternary will look like a == b ? something : something else. Value should be something : something else
            string value = o->root->getRight()->nasmExpr;
            Optimizer o2(value); //optimize the value
            o2.optimizePar(); // get rid of parentheses and things like "a = 2; void"
            value = o2.root->nasmExpr; //get the nasmExpr again, just in case it changed due to optimizing
            string tvalue = value.substr(0,value.find(':')); //true value is the expression if true
            auto tokens = Tokenizer::getLines(tvalue + ";"); //get the individual lines
            int start = line; //slightly inaccurate, but I just need things to have a unique number
            /**
             * basically, when an if statement processes, it takes the line number of the } at the end
             * of the if statement, rather than the start. I can fix it by storing an offset and then
             * applying after processing, but I am sick of this. This works just fine.
             */
            for(auto &t : tokens)
            {
                //every time we have a new line
                line -= 1;
                string s = t.second.substr(0,t.second.size()-1); // get rid of the ";"
                scope[currScope]->st[key].dependsOn.push_back(s); // this if statement depends on the lines in it
                processExpr(s, optimize, line); // we didn't process these earlier, and we need to
            }
            string fvalue = value.substr(value.find(':')+1); //get the expression if false
            
            if(fvalue != "void") //if there was an if statement
            {
                scope[currScope]->addValue(key + "_else", key, line, "signal"); //add a signal to the symbol table so we know when the else starts
                //same as above, go through the body and process the individual lines
                auto tokens = Tokenizer::getLines(fvalue + ";");
                for(auto &t : tokens)
                {
                    line -= 1;
                    string s = t.second.substr(0,t.second.size()-1);
                    scope[currScope]->st[key].dependsOn.push_back(s);
                    processExpr(s, optimize, line);
                }
            }
            line = start; //reset the line number
            //add a signal to the symbol table so we know when we are done with the if statement
            scope[currScope]->addValue(key + "_done", key, line, "signal");
            scope[currScope]->cLine[key] = "; Line " + to_string(line) + ": " + expression + "\n";
            return;
        }

        o->optimizePar(); //get rid of parenthesis
        o->process(scope[currScope]); //"replace" values with their symbol table equivalant
        //if var1 is stored at [rbp-4], then we want the Nasm Expression to signify that
        // so a tree that is var1+var2 could be [rbp-4]+[rbp-8] (if that is where they are stored)
        // I don't change the actual value, just the nasm expression value

        //if this variable has already been declared
        string type = "";
        if(o->root->getLeft()->getValue().first == "int32" || o->root->getLeft()->getValue().first == "float32")
        {
            if(scope[currScope]->exists(o->root->getLeft()->getValue().second))
            {
                throw runtime_error(o->root->getLeft()->getValue().second + " previously declared");
            }
            type = o->root->getLeft()->getValue().first;
        }
        
        string lValue = o->root->getLeft()->nasmExpr; //get the left expression
        string rValue = o->root->getRight()->nasmExpr; //get the right expression
        Optimizer o2(rValue); //we want to optimize this
        if(optimize)
            o2.optimize(); //optimize
        o2.optimizePar(); //only remove 1*(stuff) or (stuff)*1
        o2.process(scope[currScope]); //reprocess the right expression
        rValue = o2.root->nasmExpr; //replace the right expression
        //I could have done type information a lot better, but I didn't. This works though
        type = o->root->getLeft()->getValue().first == "name" ? "name" : type;
        if(!(scope[currScope]->exists(lValue)))
            type = o->root->getRight()->getValue().first == "int32" ? "int32" : type;
        type = type == "" ? (o->root->getLeft()->getValue().first == "ternary" ? "ternary" : "") : type;

        //add the value to the symbol table
        scope[currScope]->addValue(lValue, rValue, line, type);
        //comment for the lines
        scope[currScope]->cLine[lValue] = "; Line " + to_string(line) + ": " + expr + "\n";
        scope[currScope]->cLine[rValue] = "; Line " + to_string(line) + ": " + expr + "\n";
        expr = "";
        delete o; //cleanup
        o = nullptr;
    }
    /**
     * This will go through the completed symbol table and create the nasm expressions
     * 
     * @param optimize are we optimizing?
     */
    void writeToNasm(bool optimize)
    {
        // this will actually create the NASM file
        for(auto& key : scopeKeys) //go through the key starts
        {
            for(int i = 0; i < scopeCounts[key]; i++) //there might be multiple functions and scopes
            {
                //if main, just do "main:", else do something like "scope_0" and "scope_1"
                string section = key == "main" ? "main:\n" : key + to_string(i) + ":\n";
                nasm->writeString(section); //write the section header
                section = section.replace(section.size()-1,1,""); //replace the new line with nothing for the key
                string size = nasm->StackSize(scope[section]->getSize()); //get the size of this symbol table
                nasm->writeString(size); //write the stack size declaration
                string ifKey = ""; //we need to save this so we can add jump statements as necessary
                bool isElse = false;
                //go through the symbol table in insert order
                for(auto &key : scope[section]->insertOrder)
                {
                    if(key.size() > 6 && key.substr(key.size()-7) == "_change")
                    {
                        string symb = key.substr(0,key.find('_'));
                        scope[section]->st[symb].depIndex++;

                    }
                    //if the if statement is done
                    if(key.size() > 4 && key.substr(key.size()-5) == "_done")
                    {
                        string s = "";
                        if(!isElse)
                        {
                            //if we have an else, this is written into it
                            s = "." + ifKey + ":\n";
                            nasm->writeString(s);
                        }
                        //done label
                        s = "." + key + ":\n";
                        nasm->writeString(s);
                    }
                    //if we have an else
                    if(key.size() > 4 && key.substr(key.size()-5) == "_else")
                    {
                        //jump to done, this will be after the if statement
                        string s = "jmp ." + ifKey + "_done\n";
                        nasm->writeString(s);
                        //this starts the else statement
                        s = "." + ifKey + ":\n";
                        nasm->writeString(s);
                        isElse = true;
                    }
                    auto &k = scope[section]->st[key]; //get the symbol
                    if(k.print) //if we are to make NASM out of this
                    {
                        nasm->writeString(scope[section]->cLine[key]); //write the commented line
                        //functions are a bit different than everything else
                        string type = k.type == "function" ? k.dependsOn[1] : k.type;
                        Optimizer tree(k.type == "function"|| k.type == "if" ? k.dependsOn[k.depIndex] : key);
                        if(optimize)
                            tree.optimize(); //optimize the expression
                        else
                            tree.optimizePar(); //only remove parenthesis
                        //this only happens when an error happens in the try catch, and so we want to avoid Seg Faults
                        if(k.dependsOn.size() == 0)
                        {
                            continue;
                        }

                        //do one last process with the symbol table
                        tree.process(scope[currScope]);
                        //write the nasm file.
                        if(type == "if")
                        {
                            tree.processWithNasm(*nasm, to_string(k.line), nullptr, false, type);
                            ifKey = key;
                        }
                        else
                        {
                            tree.processWithNasm(*nasm, k.dependsOn[k.depIndex], nullptr, true, type, to_string(k.line));
                        }
                    }
                }
            }
        }
    }
};
int main()
{
    printf("If Elses:\n");
    Driver ifelse(ifstream("accept-ifelse-2.txt"), "accept-ifelse-2", true);
    printf("Power:\n");
    Driver power(ifstream("accept-power.txt"), "accept-power", true);
}