/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"

Token *currentToken;
Token *lookAhead;

extern Type *intType;
extern Type *charType;
extern Type *arrayType;
extern Type *floatType;
extern SymTab *symtab;
int HasReturnFunction = 0;
void scan(void)
{
  Token *tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType)
{
  if (lookAhead->tokenType == tokenType)
  {
    scan();
  }
  else
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void)
{
  Object *program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  exitBlock();
}

void compileBlock(void)
{
  Object *constObj;
  ConstantValue *constValue;

  if (lookAhead->tokenType == KW_CONST)
  {
    eat(KW_CONST);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);

      eat(SB_EQ);
      constValue = compileConstant();

      constObj->constAttrs->value = constValue;
      declareObject(constObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock2();
  }
  else
    compileBlock2();
}

void compileBlock2(void)
{
  Object *typeObj;
  Type *actualType;

  if (lookAhead->tokenType == KW_TYPE)
  {
    eat(KW_TYPE);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);

      eat(SB_EQ);
      actualType = compileType();

      typeObj->typeAttrs->actualType = actualType;
      declareObject(typeObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3();
  }
  else
    compileBlock3();
}

void compileBlock3(void)
{
  Object *varObj;
  Type *varType;

  if (lookAhead->tokenType == KW_VAR)
  {
    eat(KW_VAR);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      varType = compileType();

      varObj->varAttrs->type = varType;
      declareObject(varObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock4();
  }
  else
    compileBlock4();
}

void compileBlock4(void)
{
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void)
{
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void)
{
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE))
  {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else
      compileProcDecl();
  }
}

void compileFuncDecl(void)
{
  Object *funcObj;
  Type *returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);

  compileParams();

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  exitBlock();
}

void compileProcDecl(void)
{
  Object *procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue *compileUnsignedConstant(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue *compileConstant(void)
{
  ConstantValue *constValue;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = -constValue->intValue;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  // case TK_STRING:
  //   eat(TK_STRING);
  //   constValue = makeStringConsttant(currentToken->string[0]);
  //   break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue *compileConstant2(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_INT_CONSTANT, currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type *compileType(void)
{
  Type *type;
  Type *elementType;
  int arraySize;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type *compileBasicType(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void)
{
  if (lookAhead->tokenType == SB_LPAR)
  {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON)
    {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void)
{
  Object *param;
  Type *type;
  enum ParamKind paramKind;

  switch (lookAhead->tokenType)
  {
  case TK_IDENT:
    paramKind = PARAM_VALUE;
    break;
  case KW_VAR:
    eat(KW_VAR);
    paramKind = PARAM_REFERENCE;
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void)
{
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON)
  {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileMultipleAssignSt(void)
{
  Object *varObj;
  Type *valueType;
  Type *type;
  int hasMoreVars = 1;   // Còn biến để xử lý
  int hasMoreValues = 1; // Còn giá trị để xử lý

  // Bắt đầu xử lý danh sách biến và giá trị song song
  do
  {
    // Xử lý biến
    if (hasMoreVars)
    {
      eat(TK_IDENT);
      varObj = checkDeclaredLValueIdent(currentToken->string); // Kiểm tra biến hợp lệ
    }

    // Xử lý giá trị
    if (hasMoreValues)
    {
      if (lookAhead->tokenType == SB_ASSIGN)
      { // Bắt đầu xử lý giá trị
        eat(SB_ASSIGN);
      }
      type = compileExpression();
      checkExpressionType(type); // Lấy kiểu biểu thức
    }

    // Kiểm tra sự tương thích kiểu giữa biến và giá trị
    if (varObj != NULL && valueType != NULL)
    {
      checkTypeEquality(varObj->varAttrs->type, valueType); // Kiểm tra kiểu
    }

    // Di chuyển đến phần tử tiếp theo
    if (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA); // Đọc dấu phẩy và kiểm tra tiếp
    }
    else
    {
      hasMoreVars = 0;   // Hết biến
      hasMoreValues = 0; // Hết giá trị
    }

  } while (hasMoreVars || hasMoreValues);

  // Kiểm tra nếu số biến và số giá trị không khớp
  if (hasMoreVars != hasMoreValues)
  {
    error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
  }
}

void compileStatement(void)
{
  switch (lookAhead->tokenType)
  {
  case TK_IDENT:
    // Nếu token tiếp theo là dấu ',' => đây là gán nhiều biến
    if (lookAhead->tokenType == SB_COMMA)
    {
      compileMultipleAssignSt();
    }
    // Nếu không, đây là gán một biến thông thường
    else
    {
      compileAssignSt();
    }
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // EmptySt needs to check FOLLOW tokens
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
    // Error occurs
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type *compileLValue(void)
{
  //*** TODO: parse a lvalue (a variable, an array element, a parameter, the current function identifier)
  Object *var;
  Type *varType;
  // lamoday
  eat(TK_IDENT);
  // check if the identifier is a function identifier, or a variable identifier, or a parameter
  var = checkDeclaredLValueIdent(currentToken->string);
  Object *owner = symtab->currentScope->owner;
  if (strcmp(currentToken->string, owner->name) == 0 && lookAhead->tokenType == SB_ASSIGN)
  {
    HasReturnFunction = 1;
  }
  if (var->kind == OBJ_VARIABLE)
  {
    if (var->varAttrs->type->typeClass == TP_ARRAY)
    {
      if (lookAhead->tokenType == SB_LSEL)
      {
        varType = compileIndexes(var->varAttrs->type);
      }
      else
        varType = var->varAttrs->type;
    }
    else
    {
      varType = var->varAttrs->type;
    }
  }
  else if (var->kind == OBJ_PARAMETER)
  {
    varType = var->paramAttrs->type;
  }
  else if (var->kind == OBJ_FUNCTION)
  {
    varType = var->funcAttrs->returnType;
  }
  else
  {
    error(ERR_INVALID_LVALUE, currentToken->lineNo, currentToken->colNo);
  }

  return varType;
}

void compileCallSt(void)
{
  Object *proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  compileArguments(proc->procAttrs->paramList);
}

void compileAssign(void)
{
  switch (lookAhead->tokenType)
  {
  case SB_ASSIGN:
    eat(SB_ASSIGN);
    break;
  case SB_ASSIGN_PLUS:
    eat(SB_ASSIGN_PLUS);
    break;
  case SB_ASSIGN_SUBTRACT:
    eat(SB_ASSIGN_SUBTRACT);
    break;
  case SB_ASSIGN_TIMES:
    eat(SB_ASSIGN_TIMES);
    break;
  case SB_ASSIGN_DIVIDE:
    eat(SB_ASSIGN_DIVIDE);
    break;
  default:
    break;
  }
}

void compileAssignSt(void)
{
  //*** TODO: parse the assignment and check type consistency for multiple variables
  Type *varType;
  Type *expType;
  Object *var;
  int varCount = 0, expCount = 0;

  // Parse the list of variables
  do
  {
    eat(TK_IDENT);
    var = checkDeclaredLValueIdent(currentToken->string);
    varType = (var->kind == OBJ_VARIABLE) ? var->varAttrs->type : NULL;

    if (varCount == 0)
      checkDeclaredLValueIdent(currentToken->string);

    if (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      varCount++;
    }
    else
    {
      break;
    }
  } while (lookAhead->tokenType == TK_IDENT);

  compileAssign(); // Process assignment operator

  // Parse the list of expressions
  do
  {
    expType = compileExpression();

    if (expCount < varCount)
      checkTypeEquality(varType, expType);

    if (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      expCount++;
    }
    else
    {
      break;
    }
  } while (1);

  // Ensure the number of variables and expressions match
  if (varCount != expCount)
    error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void compileGroupSt(void)
{
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void)
{
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE)
    compileElseSt();
}

void compileElseSt(void)
{
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void)
{
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

void compileForSt(void)
{
  //*** TODO: Check type consistency of FOR's variable
  Object *var;
  Type *type;

  eat(KW_FOR);
  eat(TK_IDENT);

  // check if the identifier is a variable
  var = checkDeclaredVariable(currentToken->string);
  checkForStType(var->varAttrs->type);
  eat(SB_ASSIGN);
  type = compileExpression();
  checkForStType(type);

  checkTypeEquality(var->varAttrs->type, type);

  eat(KW_TO);
  type = compileExpression();
  checkForStType(type);
  checkTypeEquality(var->varAttrs->type, type);

  eat(KW_DO);
  compileStatement();
}

void compileArgument(Object *param)
{
  //*** TODO: parse an argument, and check type consistency
  //       If the corresponding parameter is a reference, the argument must be a lvalue
  Type *type;

  if (param->paramAttrs->kind == PARAM_VALUE)
  {
    type = compileExpression();
    checkTypeEquality(type, param->paramAttrs->type);
  }
  else
  {
    type = compileLValue();
    checkTypeEquality(type, param->paramAttrs->type);
  }
}

void compileArguments(ObjectNode *paramList)
{
  //*** TODO: parse a list of arguments, check the consistency of the arguments and the given parameters
  ObjectNode *node = paramList;

  switch (lookAhead->tokenType)
  {
  case SB_LPAR:
    eat(SB_LPAR);
    if (node == NULL)
    {
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    }
    compileArgument(node->object);
    node = node->next;

    while (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      if (node == NULL)
      {
        error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
      }
      compileArgument(node->object);
      node = node->next;
    }

    if (node != NULL)
    {
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    }

    eat(SB_RPAR);
    break;
    // Check FOLLOW set
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void)
{
  //*** TODO: check the type consistency of LHS and RSH, check the basic type
  Type *type1;
  Type *type2;

  type1 = compileExpression();
  checkBasicType(type1);

  switch (lookAhead->tokenType)
  {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  type2 = compileExpression();
  checkTypeEquality(type1, type2);
}

Type *compileExpression(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    checkExpressionType(type);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    checkExpressionType(type);
    break;
  // case TK_STRING:
  //   if (currentToken->tokenType != SB_ASSIGN)
  //   {
  //     error(ERR_OPERATOR_ASSIGN_WITH_CHAR_OR_STRING, currentToken->lineNo, currentToken->colNo);
  //   }

  //   eat(TK_STRING);
  //   type = makeArrayType(strlen(currentToken->string), makeCharType());
  //   break;
  default:
    type = compileExpression2();
  }
  return type;
}

Type *compileExpression2(void)
{
  Type *type;

  type = compileTerm();
  compileExpression3();

  return type;
}

Type *compileExpression3(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileTerm();
    checkExpressionType(type);
    // checkIntType(type);
    compileExpression3();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileTerm();
    checkExpressionType(type);
    // checkIntType(type);
    compileExpression3();
    break;
    // check the FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
  return type;
}

Type *compileTerm(void)
{
  Type *type;

  type = compileFactor();
  compileTerm2();

  return type;
}

void compileTerm2(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_TIMES:
    eat(SB_TIMES);
    type = compileFactor();
    checkExpressionType(type);
    // checkIntType(type);
    compileTerm2();
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    type = compileFactor();
    checkExpressionType(type);
    // checkIntType(type);
    compileTerm2();
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}

Type *compileFactor(void)
{
  //*** TODO: parse a factor and return the factor's type

  Object *obj;
  Type *type;
  int check = 0;

  switch (lookAhead->tokenType)
  {
  case TK_NUMBER:
    eat(TK_NUMBER);
    type = intType;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    type = charType;
    break;
  case TK_IDENT:
    if (currentToken->tokenType != SB_ASSIGN)
    {
      check = 1;
    }
    eat(TK_IDENT);
    // check if the identifier is declared
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind)
    {
    case OBJ_CONSTANT:
      switch (obj->constAttrs->value->type)
      {
      case TP_INT:
        type = intType;
        break;
      case TP_CHAR:
        type = charType;
        break;
      default:
        break;
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY)
      {
        if (lookAhead->tokenType == SB_LSEL)
        {
          type = compileIndexes(obj->varAttrs->type);
        }
        else
        {
          if (check == 1)
          {
            error(ERR_OPERATOR_ASSIGN_WITH_CHAR_OR_STRING, currentToken->lineNo, currentToken->colNo);
          }
          else
            type = obj->varAttrs->type;
        }
      }
      else
      {
        type = obj->varAttrs->type;
      }
      break;
    case OBJ_PARAMETER:
      type = obj->paramAttrs->type;
      break;
    case OBJ_FUNCTION:
      compileArguments(obj->funcAttrs->paramList);
      type = obj->funcAttrs->returnType;
      break;
    default:
      error(ERR_INVALID_FACTOR, currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }

  return type;
}

Type *compileIndexes(Type *arrayType)
{
  Type *type;
  //*** TODO: parse a sequence of indexes, check the consistency to the arrayType, and return the element type
  while (lookAhead->tokenType == SB_LSEL)
  {
    eat(SB_LSEL);
    type = compileExpression();
    checkIntType(type);
    checkArrayType(arrayType);
    arrayType = arrayType->elementType;
    eat(SB_RSEL);
  }

  checkBasicType(arrayType);
  return arrayType;
}

Type *compileSumSt(void)
{
  Type *type1;
  Type *type2;
  eat(KW_SUM);
  type1 = compileExpression();
  if (lookAhead->tokenType == SB_COMMA)
  {
    while (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      type2 = compileExpression();
      checkTypeEquality(type1, type2);
    }
  }
  else
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  return type1;
}

int compile(char *fileName)
{
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  printObject(symtab->program, 0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;
}