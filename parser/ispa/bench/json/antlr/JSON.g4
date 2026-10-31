grammar JSON;
main: json EOF;
json: '{' (STRING ':' json (',' STRING ':' json)*)? '}'
    | '[' (json (',' json)*)? ']'
    | STRING | NUMBER | BOOLEAN ;
STRING: '"' ~'"'* '"';
NUMBER: [0-9]+ ('.' [0-9]+)?;
BOOLEAN: 'true' | 'false';
WS: [ \t\r\n]+ -> skip;
