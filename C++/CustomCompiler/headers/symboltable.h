#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
/**
 * Holds the symbol tables and processes values to update the symbol table
 */
class SymbolTable
{
private:
    /**
     * holds all the meta data for a given value
     */
    class Symbol
    {
    public:
        string type; // variable type
        int dep; // how many symbols depend on this one
        bool print; // is this a symbol to be converted to NASM
        int size; // the size of this variable type
        int loc; // the location in the stack, i.e. 4 = [rbp-4]
        int line; //line number
        int depIndex;
        vector<pair<int,int>> * available; // to update when a memory location becomes available
        vector<string> dependsOn{}; // what symbols does this depend on?
        Symbol() {};
        ~Symbol() { available = nullptr; } // this pointer is shared so don't delete
        /**
         * constructor for symbol
         * 
         * @param value string representation of the value
         * @param type string representation of the type
         * @param dep how many symbols depend on this?
         * @param size how many bytes does this value need?
         * @param loc the memory location to store this at. 4 = [rbp-4], 8 = [rbp-8] etc.
         * @param available a pointer to a vector that holds available memory addresses
         */
        Symbol(string value, string type, int dep, int size, int loc, int line, vector<pair<int,int>> * available)
        {
            dependsOn.push_back(value); // this key depends on this value
            this->type = type; //the type of variable or function
            this->dep = dep; // how many expressions depends on this one?
            this->size = size; //size of variable in bytes
            this->loc = loc; //the stack spot it is assigned to
            this->available = available; //if this address becomes available, tell this thingy
            this->line = line; //the line number from the language
            this->print = dep; // 0 will be false, > 0 is true
            this->depIndex = 0; //so that we can use the correct register in variables that change
        }
        /**
         * decrement the number of dependents by 1
         */
        void dec()
        {
            dep--;
            //if nothing depends on this value, free up the memory location
            if(dep == 0)
            {
                print = false; //don't print to nasm, nothing depends on this
                available->push_back({this->loc, this->size}); //put this memory location and size into available
            }
        }
        /**
         * increment the number of dependents by 1
         */
        void inc()
        {
            dep++;
        }
        /**
         * merges the dependsOn vector with new dependents
         * 
         * @param b the vector that is merging into this, will empty it out
         */
        void Merge(vector<string> &b)
        {
            if(b.empty())
            {
                return;
            }
            dependsOn.insert(dependsOn.end(), dependsOn.begin(), dependsOn.end());
            b.clear();
        }
    };
    vector<pair<int,int>> available; // available memory locations that have previously been freed up
    int memSize = 0; // the total size of this scope
    //size map
    map<string,pair<string,int>> size {{"",{"",0}}, {"int32", {"DWORD", 4}}, {"float32", {"DWORD",4}}};
public:
    vector<string> insertOrder; // so I can sort by insert order
    map<string, Symbol> st{}; // the symbol table
    map<string, string> cLine{}; // the lines before conversion, for the purpose of comments in nasm
    vector<string> dependsOn{}; // interacts with the outside so it can update symbols with others that depend on it
    SymbolTable () {};
    /**
     * this adds a value to the symbol table
     * @param key string of the left hand of "=" or "<<"
     * @param value string of the right hand of "=" or "<<"
     * @param type string of the type of integer
     * 
     * @throws runtime errors if invalid value
     */
    void addValue(string key, string value, int line, string type = "")
    {
        // if the key is a reserved type name
        if(size.find(key) != size.end())
        {
            throw runtime_error(key + " is an invalid variable name");
        }
        // if the value is a reserved type name
        if(size.find(value) != size.end())
        {
            throw runtime_error(value + " is an invalid value");
        }
        // trying to put the wrong type into another type
        if(type == "int32" && (value.size() <= 2 && value != "0" && atoi(value.c_str()) == 0))
        {
            throw runtime_error(key + " is a " + type + ", but value " + value + " is not a " + type);
        }
        // for functions
        if(type == "function" || type == "if")
        {
            st[key] = Symbol(value, type, 1, 0, 0, line, &available);
            return;
        }
        //these are used multiple times, so might as well evaluate once
        bool keyExists = exists(key);
        bool valueExists = exists(value);
        bool addValue = true;
        int location = -999; // will be used to determine the memory location
        if(keyExists)
        {
            //if this variable has been declared before
            if(size.find(type) != size.end())
            {
                throw runtime_error(key + " has previously been declared");
            }
            // set the type to the key type
            type = st[key].type;
            // we are replacing the value, so this variable doesn't depend on anything anymore
            for(auto &s : st[key].dependsOn)
            {
                //go through all the symbols this depends on and decrement their dependents
                st[s].dec();
            }
            
            //put the new dependee into the vector
            st[key].dependsOn.push_back(value);
            st[key].depIndex++;
        }
        else // key doesn't exist
        {
            // if the variable hasn't been declared yet
            if(type == "")
            {
                throw runtime_error(key + " has not been declared and has no type");
            }
            // if we are good, put this into the insert order
            insertOrder.push_back(key);
        }
        if(valueExists) // if the value exists
        {
            
            // increment the dependants counter
            st[value].inc();
            // set this symbol's location to the value's location
            location = st[value].loc;
        }
        //value doesn't exist
        else
        {
            if(type == "name")
            {
                throw runtime_error(value + " has not been declared");
            }
            //put it into insert order
            insertOrder.push_back(value);
        }
        // this checks if a memory location is available
        for(auto &a : available)
        {
            // if memory is avaiable and the size of the type can fit into it
            if(a.second > 0 && a.second <= size[type].second)
            {
                location = a.first; //set the location to the available location
                a.first -= size[type].second; //subtract the size from the location
                a.second -= size[type].second; //subtract the size from the memory available
                //theoretically if there was a language that allows a variable to become a larger
                // variable (ie int32 to int64), this spot can be freed up
                // also, if there is a spot available and a smaller variable wants some of this memory location,
                // it can.
            }
        }
        // if the key doesn't exist, the location never updated, then increment memSize by the size
        memSize += !keyExists && location == -999 ? size[type].second : 0;
        // if location wasn't previously set, set it to memSize
        location = location == -999 ? memSize : location;

        // if key exists, the updates have already happened so no change, else make a new symbol at this key
        st[key] = keyExists? st[key] : Symbol(value, type, 0, size[type].second, location, line, &available);
        st[key].loc = location; // if the key existed, this will update the location to the proper spot
        st[key].Merge(dependsOn); // merge in any symbol that this depends on. Done in the Driver
        string loc = size[type].first + "[rbp-" + to_string(location) + "]"; // set the string location to [rbp-#]
        //if the value exists, no change, else add it to the symbol table with dep = 1, because key depends on this
        st[value] = valueExists? st[value] : Symbol(loc, type, 1, size[type].second, location, line, &available);
    }
    /**
     * get the string representation of the size, and make sure it is divisible by 16
     * 
     * @return string memSize in string format, rounded up to the nearest number divisible by 16
     */
    string getSize()
    {
        int offset = memSize % 16; //how far off from divisible from 16 are we?
        offset = offset == 0 ? 16 : offset; // make sure we turn 0 into 16
        memSize += 16 - offset; // add in however far off we are
        return to_string(memSize); //return the string
    }
    /**
     * does this key exist in the symbol table?
     * 
     * @param key to be checked for
     * @return true if key exists in symbol table
     */
    bool exists(string key)
    {
        return !st.empty() && st.find(key) != st.end();
    }
    /**
     * get a pointer to the symbol
     * 
     * @param key the key of the symbol to retrieve
     * @return pointer to the Symbol associated with the key
     */
    Symbol * getSymbol(string key)
    {
        return &st[key];
    }
};

#endif