function Tokenizer(check_string = 'int32 var3 = var1 * var2', ord = null) 
{
  this.tokens = createMap();
  this.order = ord == null ? ["comment", "nasm", "if", "type", "name", "float32", "int32", "operator", "ternary","bracket"] : ord;
  this.regex_expressions = createMap(
    [
      ["comment", /(\/\/(.)*)$/g],
      ["nasm", /(BYTE)|([A-Z]*WORD)\[[a-zA-Z\-0-9]*\]/g],
      ["if", /(if|IF)\s*\(.*\)\s*\{.*\}((else|ELSE)\s*\{.*\})*/g],
      ["ternary", /==|!=|>=|<=|(?<!<)<(?!<)|>/g],
      ["bracket", /[(){}\"\']/g],
      ["type", /([a-zA-Z_]+[a-zA-Z0-9_]*)(?=\s+[a-zA-Z_]+[a-zA-Z0-9_]*\s*=)/g],
      ["float32", /(\d+\.\d+)|(\.\d+)/g],
      ["int32", /\b\d+/g],
      ["name", /[a-zA-Z_]+[a-zA-Z0-9_]*/g],
      ["operator", /[-+=*/^;?.,]|<</g],
      ["line", /[^;]*;/g]
    ]
  );
  let s = `${check_string}`;
  this.order.forEach(key =>
  {
    let exp = [key, this.regex_expressions[key]];
    let matches = s.matchAll(exp[1]);
    for(const match of matches)
    {
      if(!this.tokens.has(`${match.index}`))
      {
        this.tokens[match.index] = [key, match[0]];
      }
    }
  });
  this.tokens = createMap([...this.tokens.entries()].sort(function(a,b){return a[0]-b[0]}))
  this.getTokens = function()
  {
    let t = [];
    this.tokens.forEach((value) =>
    {
      t.push(value);
    })
    return t;
  }

  this.printExpr = function()
  {
    let exp = "";
    let val = "";
    this.tokens.forEach(value =>
    {
      exp += `${value[0]} `;
      val += `${value[1]} `;
      if(value[0].length > value[1].length)
      {
        val += " ".repeat(value[0].length - value[1].length);
        return;
      }
      if(value[0].length < value[1].length)
      {
        exp += " ".repeat(value[1].length - value[0].length);
      }
    })
    console.log(exp);
    console.log(val);
  }

  this.printExpr();
}
function test_Tokenizer()
{
  const values = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Accept").getDataRange().getValues();
  values.forEach(row =>
  {
    const t = new Tokenizer(row[0]);
  })
}
