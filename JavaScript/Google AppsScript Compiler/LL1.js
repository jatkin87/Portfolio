/**
 * A custom Map wrapper to allow for getting and setting
 *  items with obj[key] = value, like a normal map.
 * 
 * @param {iterable} iterable - to initialize with a set of key values [[key, value], [key2, value2]...]
 * @returns Map-like object that overloads get and set
 */
function createMap(iterable = null) 
{
  const targetMap = iterable === null ? new Map() : new Map(iterable);

  return new Proxy(targetMap, 
  {
    get(target, prop, receiver) 
    {
      if (prop === 'merge') //merge function to merge maps
      {
        return (iterable) => 
        {
          for (const [key, value] of iterable) 
          {
            target.set(key, value);
          }
          return receiver;
        };
      }

      if (prop === Symbol.iterator) 
      {
        return target[Symbol.iterator].bind(target);
      }

      const value = Reflect.get(target, prop, receiver); //delegates
      if (typeof value === 'function') 
      {
        return value.bind(target);
      }

      return target.get(prop); //mapObj[key] getter
    },

    set(target, prop, value) //mapobj[key] = value
    {
      target.set(prop, value);
      return true;
    }

  });
}
/**
 * An object representing an LL1 parser algorithm. This was converted from a C++ code file that I
 *  had written for a C++ Compiler class. This JS version uses the productions found on JS Compiler
 *  sheet, and ultimately creates the rule table that will be used to determine if a language is valid.
 * 
 * @returns {Object} LL1 Object
 */
function LL1() 
{
  this.productions = createMap(); //map <string, [[string]]>
  this.productionKeys = new Set(); //[string], unique items
  this.colKeys = []; //[string]
  this.t = createMap(); //Non terminals, <string, int>
  this.table = createMap(); //map <string, map <string,int>> end table
  this.first = createMap(); //map<string,list<string>> first; //first set
  this["First"] = this.first; //linker
  this.follow = createMap(); //map<string,list<string>> follow; //follow set
  this["Follow"] = this.follow; //linker
  this.firstp = createMap(); //map<string,list<string>> firstp;  //first+ set
  this["First +"] = this.firstp; //linker
  this.start = "" //string start; //the start symbol
  this.e = "ε" //epsilon

  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const sheet = ss.getSheetByName("Productions") // Productions
  this.table = ss.getSheetByName("Table"); //rule table sheet
  this.setSheet = ss.getSheetByName("Sets"); //sets sheet
  
  const values = sheet.getDataRange().getValues(); //production values

  const columns = createMap([["eof", -1]]); //we start of rule "eof", and set it to -1
  this.colKeys.push("eof"); //this is used so that we can go through the columns in insert order
  values.forEach(production =>
  {
    //A <- B
    let lhand = production[0]; //A
    let rhand = production[1]; //B

    //initialize the production key with an array, which will contain all the Bn's
    if(typeof(this.productions[lhand]) !== typeof([])) this.productions[lhand] = [];
    
    this.productions[lhand].push([]); //individual B1, B2, etc.
    this.productionKeys.add(lhand); //for retrieving keys in insert order

    let tokens = `${rhand}`.split(" "); //split the Bn's into each tokens
    //for each token (b)
    tokens.forEach(token =>
    {
      //If the starting character is not an Alpha, or it is not uppercase (i.e. wer only want terminals here)
      if(!(/^[A-Za-z]$/.test(token.charAt(0))) || !(/^[A-Z]$/.test(token.charAt(0))))
      {
        if(!columns.has(token)) this.colKeys.push(token); //if we haven't seen this token yet, put it in column keys

        columns[token] = -1; //update the rule in columns to -1. We use this to initialize our table later on
      }
      this.productions[lhand][this.productions[lhand].length - 1].push(token); //regardless, put token into the productions
    });
  });
  
  //go through each production, and create a column with the default -1 rule code. Table will be created later.
  this.productions.forEach((value,key) =>
  {
    this.table[key] = createMap(columns);
  });
  this.t = createMap(columns); //also update t, this is just one column
  

  /** Build First Set */
  //these represent all the terminals that can be found first in a given string
  //Non terminals, use empty set
  this.productions.forEach((value, key) =>
  {
    //initialize first[key]
    if(typeof(this.first[key]) !== typeof([])) this.first[key] = [];

    this.first[key].push("{}") //push empty set
  });
  //terminals, use terminal key
  this.t.forEach((value,key) =>
  {
    if(typeof(this.first[key]) !== typeof([])) this.first[key] = [];

    this.first[key].push(key); //results in something like "name : name" or "( : (", etc.
  });

  let hasChanged = true; //once there are no changes, we are done
  while(hasChanged)
  {
    hasChanged = false; //assume a change won't happen
    
    //go through each production
    this.productions.forEach((value,key) =>
    {
      //b1, b2, b3, etc..
      value.forEach(b =>
      {
        let k = b.length; //k = b size
        let i = 0; // i = 0

        let rhs = this.first[b[0]]; //rhs = first(b[1]) <- algorithm says 1, but it just means "1st index"
        rhs = rhs.filter(item => item !== "{}"); //set our right hand side, remove the empty set
        
        //while we have epsilon AND i is less than b.length-1 (k)
        while(this.first[b[i]][0] == this.e && i < k-1)
        {
          //create our bn, filter out the empty set denoted by "{}"
          let bn = this.first[b[i+1]].filter(item => item !== "{}");
          rhs = rhs.merge(bn); //merge the sets
          i++; //increment i
        }
        //if we are at the end of bn, and first is epsilon
        if(i == k-1 && this.first[b[k-1]][0] == this.e)
        {
          rhs.push("{}"); //rhs push the empty set
        }

        let orig = [...this.first[key]]; //copy first so we can see if a change occured
        this.first[key] = [...new Set([...this.first[key], ...rhs])] //merge the first[key] with rhs
        //check if a change occured by checking if the value exists in the original.
        //if there is a value in first[key] that isn't in original, then a change has occured
        this.first[key].some(value =>
        {
          hasChanged = !(orig.includes(value)); //a change occurs if the value is not in original
          return hasChanged; //return false continues, return true breaks;
        })
      })
    })
  }
  /** End Build First Set */

  /** Build Follow Set */
  //this represents all the values that can follow a given string
  let startFound = false; //to find the start symbol
  this.productions.forEach((value,key) => //for each A Σ NT
  {
    //initialize the follow[key] to an array
    if(typeof(this.follow[key]) !== typeof([])) this.follow[key] = [];

    let c = false; //Maps don't have a some() function, so I have to simulate continue this way
    if(!startFound && key.charAt(0) === "*") //if we have found the start symbol (like *Goal)
    {
      this.follow[key].push("eof"); //Follow(S) <- {eof}
      this.start = key; //set the start production value
      startFound = true; //we found start
      c = true; //"continue"
    }
    if(!c) //we already set follow of Goal to eof, so we don't need to do this
      this.follow[key].push("{}"); //Follow(A) <- {0} empty set
  })

  hasChanged = true;
  while(hasChanged) //while the follow sets are changing
  {
    hasChanged = false; //assume no change occured
    //go through each production key in insert order
    this.productionKeys.forEach(p =>
    {
      let trailer = [...this.follow[p]]; //trailer <- follow[A]
      
      //for each b in production[p];
      this.productions[p].forEach(b =>
      {
        let k = b.length - 1; //k is set to the last valid index
        //iterate backwards, since we are concerned with follow
        for(let i = k; i >= 0; i--)
        {
          if(this.productions.has(b[i])) //if bi is non-terminal
          {
            let orig = [...this.follow[b[i]]]; //make a copy of the original
            this.follow[b[i]] = [...new Set([...this.follow[b[i]], ...trailer])]; //merge sets
            this.follow[b[i]].some(value =>
            {
              hasChanged = !(orig.includes(value));
              return hasChanged; //if changed, break, else continue;
            });

            let hasEpsilon = this.first[b[i]].includes(this.e); //does it include the epsilon symbol?

            if(hasEpsilon) //if epsilon
            {
              let U = this.first[b[i]].filter(item => item !== this.e); //create Union set and filter out the epsilon
              trailer = [...new Set([...trailer, ...U])]; //trailer U Union set
            }
            else // no epsilon
            {
              trailer = [...this.first[b[i]]]; //trailer is first[bi]
            }
          }
          else //bi is a terminal
          {
            trailer = [...this.first[b[i]]]; //trailer is first[bi]
          }
        }
      })
    })
  }
  this.follow[this.e] = this.first[this.e]; //add epsilon to follow

  /** Build First Plus */
  //combining first and follow sets
  this.first.forEach((value, key) =>
  {
    this.firstp[key] = value; //
    if(value.includes(this.e)) //if first[key] includes epsilon
    {
      this.firstp[key] = [...new Set([...this.firstp[key], ...this.follow[key]])]; //combine first and follow
      this.firstp[key].sort(); //sanity check to compare to C++ version of this code.
    }
  });
  /** End Build First Plus */

  /** Build Table */
  //This creates the rule table that the tokenizer will follow to determine if language is valid
  this.productionKeys.forEach(key =>
  {
    let rule = 0; //rule = 0 means go to this key rule
    //go through each bn
    this.productions[key].forEach(b =>
    {
      //go through each word in first plus b0, specifically b0
      this.firstp[b[0]].some(w =>
      {
        if(w === "{}") //if empty set
        {
          return false; //continue
        }
        if(w === this.e) //if epsilon
        {
          //go through each table entry for first + at the given key
          this.firstp[key].some(t =>
          {
            if(t === this.e || t === "{}") //if epsilon or empty set
            {
              return false; //continue;
            }
            this.table[key][t] = this.table[key][t] == -1 ? rule : this.table[key][t]; //update rule if needed
          })
          return false; //continue
        }
        this.table[key][w] = this.table[key][w] == -1 ? rule : this.table[key][w]; //update the rule
      })
      rule++; //increment rule
    });
  });

  /** Printing functions */
  /**
   * printTable uses the Table sheet to display the rule table created from the algorithm. If Table Sheet
   *  already contains values, this function returns without doing anything.
   */
  this.printTable = function()
  {
    //const a1 = this.table.getRange(1,1).getValue();
    //if(a1 !== "") return; //if the table is already populated, return
    this.table.clear(); //I decided just to clear the sheet and print every time

    this.table.appendRow(["Table:"]); //append table title
    let s = [""]; //header row, with empty string for the first cell
    this.colKeys.some(col =>
    {
      if(col == this.e) return false; //don't worry about epsilon
      s.push(`'${col}`); // column name
    });
    this.table.appendRow(s); //append the column names row

    s = []; //reset for the next rows
    let i = 0; 
    //go through each production key
    this.productionKeys.forEach(value =>
    {
      s.push(`'${value}`); //push the key into the row
      let max = -1; //internally, I do the rules differently than the algroithm calls for so this is to normalize
      //go through each column key in insert order
      this.colKeys.some(col =>
      {
        if(col === this.e) return false; //ignore epsilon

        if(this.table[value][col] == -1)
        {
          s.push("-"); //replace -1 rule with -, so it looks nicer
        }
        else
        {
          s.push(`'${this.table[value][col] + i}`); //else do the rule
        }
      })
      this.table.appendRow(s); //append the row
      s = [] //reset the row
      i += max + 1; //to calculate the rule number so it looks normalized to the actual algorithm
    })
  };
  /**
   * printSet(name) will print a set given a name. The only valid values are
   *  "First", "first", "Follow", "follow", "First +", "firstp".
   */
  this.printSet = function (name)
  {
    const valid = ["First", "first", "Follow", "follow", "First +", "firstp"];
    if(!valid.includes(name)) throw new Error(`${name} is not a valid set`);
    
    this.setSheet.appendRow([`${name}:`]);
    const A = this[name];
    let printArr = [];
    let k = 0
    A.forEach((value,key) =>
    {
      if(typeof(printArr[0]) !== typeof([])) printArr[0] = [];
      printArr[0].push(`'${key}`);
      let i = 1;
      value.some(t =>
      {
        if(t === "{}") return false;
        if(i >= printArr.length) 
        {
          printArr.push([]);
          for(let j = 1; j <= k; j++)
          {
            printArr[i].push("~");
          }
        }
        else
        {
          let s = k - printArr[i].length;
          for(let j = 0; j < s; j++)
          {
            printArr[i].push("~");
          }
        }
        printArr[i].push(`'${t}`);
        i++;
      })
      k++;
    })

    let max = -1;
    printArr.forEach(row =>
    {
      max = max < row.length ? row.length : max;
      for(let i = row.length; i < max; i++)
      {
        row.push("~"); // make sure we fill to the end of the row 
      }
      this.setSheet.appendRow(row);
      
    })
    this.setSheet.appendRow([" "]) //create a space;
  };
  /** prints all sets (First, Follow, and First +) to the Sheet named "Sets".
   *  Clears old sheet and reprints when ran.
   */
  this.printSets = function ()
  {
    this.setSheet.clear();
    this.printSet("First");
    this.printSet("Follow");
    this.printSet("First +");
  };
  return this;
}