%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */

%x COMMENT STR

%%

 /*
  * TODO: Put your codes here (lab2).
  *
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg.Newline();}

 /* reserved words */
"," {adjust();return Parser::COMMA;}
":" {adjust();return Parser::COLON;}
";" {adjust();return Parser::SEMICOLON;}
"(" {adjust();return Parser::LPAREN;}
")" {adjust();return Parser::RPAREN;}
"[" {adjust();return Parser::LBRACK;}
"]" {adjust();return Parser::RBRACK;}
"{" {adjust();return Parser::LBRACE;}
"}" {adjust();return Parser::RBRACE;}
"." {adjust();return Parser::DOT;}
"+" {adjust();return Parser::PLUS;}
"-" {adjust();return Parser::MINUS;}
"*" {adjust();return Parser::TIMES;}
"/" {adjust();return Parser::DIVIDE;}
"=" {adjust();return Parser::EQ;}
"<>" {adjust();return Parser::NEQ;}
"<" {adjust();return Parser::LT;}
"<=" {adjust();return Parser::LE;}
">" {adjust();return Parser::GT;}
">=" {adjust();return Parser::GE;}
"&" {adjust();return Parser::AND;}
"|" {adjust();return Parser::OR;}
":=" {adjust();return Parser::ASSIGN;}
"array" {adjust(); return Parser::ARRAY;}
"if" {adjust();return Parser::IF;}
"then" {adjust();return Parser::THEN;}
"else" {adjust();return Parser::ELSE;}
"while" {adjust();return Parser::WHILE;}
"for" {adjust();return Parser::FOR;}
"to" {adjust();return Parser::TO;}
"do" {adjust();return Parser::DO;}
"let" {adjust();return Parser::LET;}
"in" {adjust();return Parser::IN;}
"end" {adjust();return Parser::END;}
"of" {adjust();return Parser::OF;}
"break" {adjust();return Parser::BREAK;}
"nil" {adjust();return Parser::NIL;}
"function" {adjust();return Parser::FUNCTION;}
"var" {adjust();return Parser::VAR;}
"type" {adjust();return Parser::TYPE;}


[a-zA-Z][a-zA-Z0-9_]* {
  adjust();
  Scanner::stringBuf_=matched();
  return Parser::ID;
  }

[0-9]+  {
  adjust();
  Scanner::stringBuf_= matched();
  return Parser::INT;
  }

\"  {
  adjust();
  Scanner::stringBuf_="";
  begin(StartCondition__::STR);
  }

"/*"  {
  adjust();
  Scanner::commentLevel_=1;
  begin(StartCondition__::COMMENT);
}

<STR> {
  \"  {
    Scanner::charPos_+=1;
    setMatched(Scanner::stringBuf_);
    begin(StartCondition__::INITIAL);
    return Parser::STRING;
  }

  \\n {
    Scanner::adjustStr();
    Scanner::stringBuf_+='\n';
  }

  \\t {
    Scanner::adjustStr();
    Scanner::stringBuf_+='\t';
  }

  \\\\  {
    Scanner::adjustStr();
    Scanner::stringBuf_+='\\';
  }

  \\\"  {
    Scanner::adjustStr();
    Scanner::stringBuf_+='\"';
  }

  \\[ \n\t\f]+\\  {
    Scanner::adjustStr();
  }

  \\[0-9]{3}  {
    Scanner::adjustStr();

    std::string match=matched();
    int offset=std::stoi(match.substr(1,4))-97;
    Scanner::stringBuf_+=(char)('a'+offset);
  }
  
  \\\^[A-Z] {
    Scanner::adjustStr();

    std::string match=matched();
    
    Scanner::stringBuf_+=(char)(match[2]-'A'+1);
  }

  . {
    Scanner::adjustStr();
    std::string match=matched();
    Scanner::stringBuf_+=match;
  
  }

}

<COMMENT> {
  "*/"  {
    adjust();
    Scanner::commentLevel_--;
    if(Scanner::commentLevel_==0){
      begin(StartCondition__::INITIAL);
    }
  }

  "/*"  {
    adjust();
    Scanner::commentLevel_++;
  }
  
  "\n"  {
    adjust();
    errormsg.Newline();
  }

  . {
    more();
  }
}



. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}



