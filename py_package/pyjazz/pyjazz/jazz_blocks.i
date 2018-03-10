%module jazz_blocks

%{
extern char * expand_escaped(char * buff);
extern int count_utf8_bytes(char * buff, int len);
%}

extern char * expand_escaped(char * buff);
extern int count_utf8_bytes(char * buff, int len);
