#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <setjmp.h>

/*  
    assignment: identifier = assignment
    term: factor (("+" | "-") factor)*
    factor: unary (("/" | "*") unary)*
    unary: ("-" | "+") unary | literal
    primary: number | '(' assignment ')' | identifer

    identifier = [a-zA-z]
    number: [0-9]
*/

#define RESET_TEXT_COLOR() printf("\033[0m")
#define SET_GREEN_TEXT_COLOR() printf("\033[1m\033[32m")
#define SET_RED_TEXT_COLOR() printf("\033[1m\033[31m")

#define AS(src, dest) ((dest)(src))

// ===== Table =====
// (Implemented with BST)

typedef struct _table_node{
    char* key;
    unsigned int key_length;

    float value;

    struct _table_node* left, *right;
}table_node_t;

typedef struct _table_node table_t;

table_t* init_table(){
    return NULL;
}

table_node_t* new_table_node(const char* key, unsigned int length, double value){
    table_node_t* node = (table_node_t*)malloc(sizeof(table_node_t));

    node->key = (char*)malloc(sizeof(char) * length + 1);
    memcpy(node->key, key, length);
    
    node->key_length = length;
    node->value = value;

    node->right = NULL;
    node->left = NULL;

    return node;
}

void table_destroy(table_t* table){
    if(table == NULL) return;

    table_destroy(table->left);
    table_destroy(table->right);

    free(table->key);
    free(table);
}

bool table_insert(table_t** t, const char* key, unsigned int length, float value){

    if(*t == NULL){
        *t = new_table_node(key, length, value);
        return true;
    }

    if(strncmp(key, (*t)->key, length) > 0){
        return table_insert(&(*t)->right, key, length, value);
    }else{
        return table_insert(&(*t)->left, key, length, value);
    }

    return false;
}

bool table_get(table_t* t, const char* key, unsigned int length, float* result){
    if(t == NULL) return false;
    
    if(strncmp(key, t->key, length) == 0){
        *result = t->value;
        return true;
    }else if(strncmp(key, t->key, length) > 0){
        return table_get(t->right, key, length, result);
    }else{
        return table_get(t->left, key, length, result);
    }

    return false;
}

// ===== Error Manger =====

#define UNSAFE(err) if(setjmp(err.error_buf) == 0)
#define THROW(err) longjmp(err.error_buf, 1)

typedef struct _error_manager{
    bool had_error;
    jmp_buf error_buf;

    void (*emit_error)(struct _error_manager*, const char*, ...);
    void (*reset)(struct _error_manager*);
}error_manager_t;

void error_manager_emit_error(error_manager_t* e, const char* fmt, ...){
    va_list args;

    va_start(args, fmt);

    SET_RED_TEXT_COLOR();
    vprintf(fmt, args);
    RESET_TEXT_COLOR();
    
    va_end(args);

    e->had_error = true;
    THROW((*e));
}

void error_manager_reset(error_manager_t *e){
    e->had_error = false;
}

error_manager_t create_new_error_manager(){
    return (error_manager_t){
        .emit_error = error_manager_emit_error,
        .reset = error_manager_reset,
        .had_error = false
    };
}

// ===== Tokenizer =====

typedef struct _token{
    enum {
        T_ERROR = 1,
        T_IDENTIFIER,
        T_NUMBER,
        T_PLUS,
        T_EQUAL,
        T_MINUS,
        T_STAR,
        T_SLASH,
        T_LPAREN,
        T_RPAREN,
        T_EOF,
    } token_type;
    
    const char* lexeme;
    int length;
    
} token_t;

typedef struct _tokenizer{
    const char* source;

    int current;
    int start;

    token_t (*scan_token)(struct _tokenizer*);
    char (*advance)(struct _tokenizer*);
    void (*reset)(struct _tokenizer*);
}tokenizer_t;

#define RETURN_TOKEN(type, t) do{ \
    return (token_t) { \
        .token_type = type, \
        .length = (t->current - t->start), \
        .lexeme = (t->source + t->start) \
    }; \
}while(0)

#define RETURN_TOKEN_ERROR(message, t) do{ \
    return (token_t) { \
        .token_type = T_ERROR, \
        .length = -1, \
        .lexeme = message \
    }; \
}while(0)

token_t tokenizer_scan_token(tokenizer_t* t){

    while(t->source[t->current] == ' ') t->advance(t);

    t->start = t->current;
    char c = t->advance(t);
    
    switch (c){
        case '\0':
        case '\n':
            RETURN_TOKEN(T_EOF, t);
        case '+':
            RETURN_TOKEN(T_PLUS, t);
        case '-':
            RETURN_TOKEN(T_MINUS, t);
        case '/':
            RETURN_TOKEN(T_SLASH, t);
       case '*':
            RETURN_TOKEN(T_STAR, t);
        case '(':
            RETURN_TOKEN(T_LPAREN, t);
        case ')':
            RETURN_TOKEN(T_RPAREN, t);
        case '=':
            RETURN_TOKEN(T_EQUAL, t);   
        default:
            if(isdigit(c)){
                while(isdigit(t->source[t->current])){
                    t->advance(t);
                }

                if(t->source[t->current] == '.'){
                    t->advance(t);
                    while(isdigit(t->source[t->current])){
                        t->advance(t);
                    }   
                }

                RETURN_TOKEN(T_NUMBER, t);
            }

            if(isalpha(c)){
                while(isalpha(t->source[t->current])){
                    t->advance(t); 
                }

                RETURN_TOKEN(T_IDENTIFIER, t);
            }

            break;
    }

    RETURN_TOKEN_ERROR("Unrecognized token", t);
}

char tokenizer_advance(tokenizer_t* t){
    return (t->source[t->current] != '\n') ? t->source[t->current++] : '\0';
}

void tokenizer_reset(tokenizer_t* t){
    t->start = 0;
    t->current = 0;
}

tokenizer_t initialize_tokenizer(const char* source){
    return (tokenizer_t) {
        .source = source,
        .current = 0,
        .start = 0,
        .scan_token = tokenizer_scan_token,
        .advance = tokenizer_advance,
        .reset = tokenizer_reset
    };
}

// ===== Parser & AST =====

typedef struct _expr{
    enum {
        EXPR_LITERAL,
        EXPR_VARIABLE,
        EXPR_ASSIGN,
        EXPR_UNARY,
        EXPR_BINARY,
        EXPR_GROUPING
    } expr_type;
}expr_t;

typedef struct _binary_expr{
    expr_t base;
    
    expr_t* left;
    expr_t* right;

    char op;
}binary_expr_t;

typedef struct _unary_expr{
    expr_t base;
    char op;
    expr_t* expression;
}unary_expr_t;

typedef struct _literal_expr{
    expr_t base;
    float number;
}literal_expr_t;

typedef struct _variable_expr{
    expr_t base;
    token_t name;
}variable_expr_t;

typedef struct _assign_expr{
    expr_t base;
    
    token_t name;
    expr_t* value;
}assign_expr_t;

typedef struct _grouping_expr{
    expr_t base;
    expr_t* expression;
}grouping_expr_t;

expr_t* create_literal(float number){
    literal_expr_t* e = (literal_expr_t*)malloc(sizeof(literal_expr_t));
    e->base.expr_type = EXPR_LITERAL;
    e->number = number;

    return AS(e, expr_t*);
}

expr_t* create_variable(token_t name){
    variable_expr_t* e = (variable_expr_t*)malloc(sizeof(variable_expr_t));
    e->base.expr_type = EXPR_VARIABLE;
    e->name = name;

    return AS(e, expr_t*);
}

expr_t* create_assign(token_t name, expr_t* value){
    assign_expr_t* e = (assign_expr_t*)malloc(sizeof(assign_expr_t));
    e->base.expr_type = EXPR_ASSIGN;
    e->name = name;
    e->value = value;

    return AS(e, expr_t*);
}

expr_t* create_binary(char op, expr_t* left, expr_t* right){
    binary_expr_t* e = (binary_expr_t*)malloc(sizeof(binary_expr_t));
    e->base.expr_type = EXPR_BINARY;
    e->left  = left;
    e->right = right;
    e->op = op;

    return AS(e, expr_t*);
}

expr_t* create_unary(char op, expr_t* right){
    unary_expr_t* e = (unary_expr_t*)malloc(sizeof(unary_expr_t));
    e->base.expr_type = EXPR_UNARY;
    e->expression = right;
    e->op = op;  

    return AS(e, expr_t*);
}

expr_t* create_grouping(expr_t* expr){
    grouping_expr_t* e = (grouping_expr_t*)malloc(sizeof(grouping_expr_t));
    e->base.expr_type = EXPR_GROUPING;
    e->expression = expr;

    return AS(e, expr_t*);
}

typedef struct _parser{
    error_manager_t err_manager;    
    tokenizer_t* tokenizer;

    token_t current_token, prev_token;
    
    float (*convert_lexeme_to_number)(struct _parser*);

    void (*get_token)(struct _parser*);
    bool (*match)(struct _parser*, int type);
    expr_t* (*parse)(struct _parser*);

}parser_t;

expr_t* parse_assign(parser_t*);
expr_t* parse_term(parser_t*);
expr_t* parse_factor(parser_t*);
expr_t* parse_unary(parser_t*);
expr_t* parse_primary(parser_t*);

expr_t* parse_assign(parser_t* p){
    expr_t* e = parse_term(p);

    if(p->match(p, T_EQUAL)){
        if(e->expr_type != EXPR_VARIABLE){
            p->err_manager.emit_error(
                &p->err_manager,
                "Expect variable name.\n"
            );
            return NULL;
        }

        expr_t* value = parse_assign(p);
        
        token_t name = AS(e, variable_expr_t*)->name;
        free(e);

        return create_assign(name, value);
    }

    return e;
}

expr_t* parse_term(parser_t* p){
    expr_t* expr = parse_factor(p);
    
    while (p->match(p, T_PLUS) || p->match(p, T_MINUS)){
        char op = *p->prev_token.lexeme;
        expr_t* right = parse_factor(p);

        expr = create_binary(op, expr, right);
    }

    return expr;
}

expr_t* parse_factor(parser_t* p){
    expr_t* expr = parse_unary(p);
    
    while (p->match(p, T_SLASH) || p->match(p, T_STAR)){
        char op = *p->prev_token.lexeme;
        expr_t* right = parse_unary(p);

        expr = create_binary(op, expr, right);
    }

    return expr;
}

expr_t* parse_unary(parser_t* p){

    if(p->match(p, T_MINUS) || p->match(p, T_PLUS)){
        char op = *p->prev_token.lexeme;
        expr_t* right = parse_unary(p);
        return create_unary(op, right);
    }

    return parse_primary(p);
}

expr_t* parse_primary(parser_t* p){
    if(p->match(p, T_NUMBER)) {
        return create_literal(p->convert_lexeme_to_number(p));
    }

    if (p->match(p, T_IDENTIFIER)){
        return create_variable(p->prev_token);
    }
    

    if(p->match(p, T_LPAREN)){
        expr_t* expr = parse_assign(p);

        if(!p->match(p, T_RPAREN)){
            p->err_manager.emit_error(&p->err_manager, "Unclosed \'(\'.\n");
            return NULL;
        }

        return create_grouping(expr);
    }

    return NULL;
}

float parser_convert_lexeme_to_number(struct _parser* p){
    token_t t = p->prev_token;
    return strtof(t.lexeme, NULL);
}

void parser_get_token(parser_t* p){

    p->prev_token = p->current_token;
    p->current_token = p->tokenizer->scan_token(p->tokenizer);

    if(p->current_token.token_type == T_ERROR){
        p->err_manager.emit_error(
            &p->err_manager, 
            "Error: %s\n", p->current_token.lexeme
        );
    }
}

bool parser_match(parser_t* p, int token_type){
    if (p->current_token.token_type == token_type){
        p->get_token(p);
        return true;
    }  

    return false;
}

expr_t* parser_parse(parser_t* p){
    
    expr_t* e = NULL;
    p->err_manager.had_error = false;

    UNSAFE(p->err_manager) {
        p->get_token(p); // get first token
        e = parse_assign(p);
    }

    p->tokenizer->reset(p->tokenizer);

    return e;
}

parser_t initialize_parser(tokenizer_t* t){
    parser_t p = {
        .tokenizer = t,
        .match = parser_match,
        .parse = parser_parse,
        .convert_lexeme_to_number = parser_convert_lexeme_to_number,
        .get_token = parser_get_token,
        .err_manager = create_new_error_manager()
    };

    return p;
}

// ===== Interpreter =====

typedef struct _interpreter{    
    table_t* variables;

    error_manager_t err_manager;

    float (*evaluate_ast)(struct _interpreter*, expr_t*);
    void (*destroy_ast)(expr_t*);
}interpreter_t;

void interpreter_destroy_ast(expr_t* ast){
    if(ast == NULL) return;

    switch (ast->expr_type){
        case EXPR_BINARY:
            interpreter_destroy_ast(AS(ast, binary_expr_t*)->left);
            interpreter_destroy_ast(AS(ast, binary_expr_t*)->right);
            break;
        case EXPR_GROUPING:
            interpreter_destroy_ast(AS(ast, grouping_expr_t*)->expression);
            break;
        case EXPR_UNARY:
            interpreter_destroy_ast(AS(ast, unary_expr_t*)->expression);
            break;
        case EXPR_ASSIGN:
            interpreter_destroy_ast(AS(ast, assign_expr_t*)->value);
            break;
        case EXPR_VARIABLE:
            break;
        default:
            break;
    }

    free(ast);
}

float interpreter_evaluate_ast(interpreter_t* i, expr_t* ast){
    if(ast == NULL){
        i->err_manager.emit_error(
            &i->err_manager,
            "Unable to interpret this string.\n"
        );
        return NAN;
    }
    
    float left, right;

    switch(ast->expr_type){
        case EXPR_LITERAL:
            return AS(ast, literal_expr_t*)->number;
        case EXPR_VARIABLE: {
            token_t name = AS(ast, variable_expr_t*)->name;

            if(!table_get(i->variables, name.lexeme, name.length, &left)){
                i->err_manager.emit_error(
                    &i->err_manager,
                    "\'%.*s\' variable not defined.\n", 
                    name.length, name.lexeme
                );
            }
        
            return left;
        }
        case EXPR_UNARY: {
            unary_expr_t* e = AS(ast, unary_expr_t*);

            right = i->evaluate_ast(i, e->expression);

            return (e->op == '+') ? right : -right; 
        }
        case EXPR_BINARY: {
            binary_expr_t* e = AS(ast, binary_expr_t*);

            left = i->evaluate_ast(i, e->left);
            right = i->evaluate_ast(i, e->right);

            switch (e->op){
                case '+':
                    return left + right;
                case '-':
                    return left - right;
                case '/':
                    if(right == 0){
                        i->err_manager.emit_error(
                            &i->err_manager,
                            "Zero division error, unable to divide by 0.\n"
                        );
                        return NAN;
                    }
                    return left / right;
                case '*':
                    return left * right;
                default:
                    return NAN;
            }
        }
        case EXPR_GROUPING: 
            return i->evaluate_ast(i, AS(ast, grouping_expr_t*)->expression);
        case EXPR_ASSIGN: {
            assign_expr_t* e = AS(ast, assign_expr_t*);
            left = i->evaluate_ast(i, e->value);

            if(!table_insert(&i->variables, e->name.lexeme, e->name.length, left)){
                i->err_manager.emit_error(
                    &i->err_manager,
                    "Unable to define variable \'%.*s\'.\n", 
                    e->name.length, e->name.lexeme
                );
            }

            return left;
        }
    }

    return NAN;
}


interpreter_t* initialize_interpreter(){
    interpreter_t* i = (interpreter_t*)malloc(sizeof(interpreter_t));

    i->destroy_ast = interpreter_destroy_ast;
    i->evaluate_ast = interpreter_evaluate_ast;
    i->variables = init_table(); 
    i->err_manager = create_new_error_manager();

    return i;  
}

void destroy_interpreter(interpreter_t* i){
    table_destroy(i->variables);
    free(i);
}

int main(void){

    char buffer[1024] = {0};
    
    tokenizer_t tokenzier = initialize_tokenizer(buffer);
    interpreter_t* interpreter = initialize_interpreter();
    parser_t parser = initialize_parser(&tokenzier);

    while(1){

        SET_GREEN_TEXT_COLOR();
        printf("math -> ");
        RESET_TEXT_COLOR();

        fgets(buffer, 1024, stdin);

        if(*buffer == '\n') continue;

        if(strncmp(buffer, "_quit", 5) == 0){
            printf("\n:) Bye...\n");
            break;
        }

        expr_t* ast = parser.parse(&parser);

        if(!parser.err_manager.had_error){
            UNSAFE(interpreter->err_manager){
                float result = interpreter->evaluate_ast(interpreter, ast);
                printf("%g\n", result);
            }
        }

        interpreter->destroy_ast(ast);
        interpreter->err_manager.reset(&interpreter->err_manager);
    }
    
    destroy_interpreter(interpreter);

    return 0;
}