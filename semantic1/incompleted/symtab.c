/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

void freeObject(Object *obj);
void freeScope(Scope *scope);
void freeObjectList(ObjectNode *objList);
void freeReferenceList(ObjectNode *objList);

SymTab *symtab;
Type *intType;
Type *charType;

/******************* Type utilities ******************************/

Type *makeIntType(void)
{
  Type *type = (Type *)malloc(sizeof(Type));
  type->typeClass = TP_INT;
  return type;
}

Type *makeCharType(void)
{
  Type *type = (Type *)malloc(sizeof(Type));
  type->typeClass = TP_CHAR;
  return type;
}

Type *makeArrayType(int arraySize, Type *elementType)
{
  Type *type = (Type *)malloc(sizeof(Type));
  type->typeClass = TP_ARRAY;
  type->arraySize = arraySize;
  type->elementType = elementType;
  return type;
}

Type *duplicateType(Type *type)
{
  // TODO
  Type *newType = (Type *)malloc(sizeof(Type));
  newType->typeClass = type->typeClass;
  if (type->typeClass == TP_ARRAY)
  {
    newType->arraySize = type->arraySize;
    newType->elementType = duplicateType(type->elementType);
  }
  return newType;
}

int compareType(Type *type1, Type *type2)
{
  // TODO
  if (type1->typeClass != type2->typeClass)
  {
    return 0;
  }
  else
  {
    if (type1->typeClass == TP_ARRAY)
    {
      if (type1->arraySize != type2->arraySize)
        return 0;
      if (type1->arraySize != type2->arraySize)
        return 0;
      if (!compareType(type1->elementType, type2->elementType))
        return 0;
    }
    return 1;
  }
}

void freeType(Type *type)
{
  // TODO
  if (type != NULL)
  {
    if (type->typeClass == TP_ARRAY)
    {
      freeType(type->elementType);
    }
    free(type);
  }
}

/******************* Constant utility ******************************/

ConstantValue *makeIntConstant(int i)
{
  // TODO
  ConstantValue *constant = (ConstantValue *)malloc(sizeof(ConstantValue));
  constant->type = TP_INT;
  constant->intValue = i;
  return constant;
}

ConstantValue *makeCharConstant(char ch)
{
  // TODO
  ConstantValue *constant = (ConstantValue *)malloc(sizeof(ConstantValue));
  constant->type = TP_CHAR;
  constant->charValue = ch;
  return constant;
}

ConstantValue *duplicateConstantValue(ConstantValue *v)
{
  // TODO
  ConstantValue *newConstant = (ConstantValue *)malloc(sizeof(ConstantValue));
  newConstant->type = v->type;
  if (v->type == TP_INT)
    newConstant->intValue = v->intValue;
  if (v->type == TP_CHAR)
    newConstant->charValue = v->charValue;
  return newConstant;
}

/******************* Object utilities ******************************/

Scope *createScope(Object *owner, Scope *outer)
{
  Scope *scope = (Scope *)malloc(sizeof(Scope));
  scope->objList = NULL;
  scope->owner = owner;
  scope->outer = outer;
  return scope;
}

Object *createProgramObject(char *programName)
{
  Object *program = (Object *)malloc(sizeof(Object));
  strcpy(program->name, programName);
  program->kind = OBJ_PROGRAM;
  program->progAttrs = (ProgramAttributes *)malloc(sizeof(ProgramAttributes));
  program->progAttrs->scope = createScope(program, NULL);
  symtab->program = program;

  return program;
}

Object *createConstantObject(char *name)
{
  // TODO
  Object *constant = (Object *)malloc(sizeof(Object));
  strcpy(constant->name, name);
  constant->kind = OBJ_CONSTANT;
  constant->constAttrs = (ConstantAttributes *)malloc(sizeof(ConstantAttributes));
  constant->constAttrs->value = (ConstantValue *)malloc(sizeof(ConstantValue));
  return constant;
}

Object *createTypeObject(char *name)
{
  // TODO
  Object *type = (Object *)malloc(sizeof(Object));
  strcpy(type->name, name);
  type->kind = OBJ_TYPE;
  type->typeAttrs = (TypeAttributes *)malloc(sizeof(TypeAttributes));
  type->typeAttrs->actualType = (Type *)malloc(sizeof(Type));
  return type;
}

Object *createVariableObject(char *name)
{
  // TODO
  Object *var = (Object *)malloc(sizeof(Object));
  strcpy(var->name, name);
  var->kind = OBJ_VARIABLE;
  var->varAttrs = (VariableAttributes *)malloc(sizeof(VariableAttributes));
  var->varAttrs->type = (TypeAttributes *)malloc(sizeof(TypeAttributes));
  var->varAttrs->scope = createScope(var, symtab->currentScope);
  return var;
}

Object *createFunctionObject(char *name)
{
  // TODO
  Object *func = (Object *)malloc(sizeof(Object));
  strcpy(func->name, name);
  func->kind = OBJ_FUNCTION;
  func->funcAttrs = (FunctionAttributes *)malloc(sizeof(FunctionAttributes));
  func->funcAttrs->paramList = (ParameterAttributes *)malloc(sizeof(ParameterAttributes));
  func->funcAttrs->returnType = (TypeAttributes *)malloc(sizeof(TypeAttributes));
  func->funcAttrs->scope = createScope(func, symtab->currentScope);
  return func;
}

Object *createProcedureObject(char *name)
{
  // TODO
  Object *proce = (Object *)malloc(sizeof(Object));
  strcpy(proce->name, name);
  proce->kind = OBJ_PROCEDURE;
  proce->procAttrs = (ProcedureAttributes *)malloc(sizeof(ProcedureAttributes));
  proce->procAttrs->paramList = (ParameterAttributes *)malloc(sizeof(ParameterAttributes));
  proce->procAttrs->scope = createScope(proce, symtab->currentScope);
  return proce;
}

Object *createParameterObject(char *name, enum ParamKind kind, Object *owner)
{
  // TODO
  Object *param = (Object *)malloc(sizeof(Object));
  strcpy(param->name, name);
  param->kind = OBJ_PARAMETER;
  param->paramAttrs->kind = kind;
  param->paramAttrs->function = (FunctionAttributes *)malloc(sizeof(FunctionAttributes));
  param->paramAttrs->type = (TypeAttributes *)malloc(sizeof(TypeAttributes));
  param->paramAttrs->function = owner;
  return param;
}

void freeObject(Object *obj)
{
  // TODO
  switch (obj->kind)
  {
  case OBJ_PROGRAM:
    freeScope(obj->procAttrs->scope);
    free(obj->progAttrs);
    free(obj);
    break;
  case OBJ_CONSTANT:
    free(obj->constAttrs->value);
    free(obj->constAttrs);
    free(obj);
    break;
  case OBJ_TYPE:
    freeType(obj->typeAttrs->actualType);
    free(obj->typeAttrs);
    free(obj);
    break;
  case OBJ_VARIABLE:
    freeScope(obj->varAttrs->scope);
    freeType(obj->varAttrs->type);
    free(obj->varAttrs);
    free(obj);
    break;
  case OBJ_FUNCTION:
    freeObjectList(obj->funcAttrs->paramList);
    freeType(obj->funcAttrs->returnType);
    freeScope(obj->funcAttrs->scope);
    free(obj->funcAttrs);
    free(obj);
    break;
  case OBJ_PROCEDURE:
    freeObjectList(obj->procAttrs->paramList);
    freeScope(obj->procAttrs->scope);
    free(obj->procAttrs);
    free(obj);
    break;
  case OBJ_PARAMETER:
    freeType(obj->paramAttrs->type);
    freeObject(obj->paramAttrs->function);
    free(obj->paramAttrs);
    free(obj);
    break;
  }
}

void freeScope(Scope *scope)
{
  // TODO
  freeObjectList(scope->objList);
  free(scope);
}

void freeObjectList(ObjectNode *objList)
{
  // TODO
  ObjectNode *tempNode = objList;
  while (tempNode != NULL)
  {
    ObjectNode *node = tempNode;
    tempNode = tempNode->next;
    free(node);
  }
}

void freeReferenceList(ObjectNode *objList)
{
  // TODO
  ObjectNode *tempNode = objList;
  while (tempNode != NULL)
  {
    ObjectNode *node = tempNode;
    tempNode = tempNode->next;
    free(node);
  }
}

void addObject(ObjectNode **objList, Object *obj)
{
  ObjectNode *node = (ObjectNode *)malloc(sizeof(ObjectNode));
  node->object = obj;
  node->next = NULL;
  if ((*objList) == NULL)
    *objList = node;
  else
  {
    ObjectNode *n = *objList;
    while (n->next != NULL)
      n = n->next;
    n->next = node;
  }
}

Object *findObject(ObjectNode *objList, char *name)
{
  // TODO
  for (ObjectNode *o = objList; o != NULL; o = o->next)
  {
    if (strcmp(o->object->name, name) == 0)
    {
      return o->object;
    }
  }
  return NULL;
}

/******************* others ******************************/

void initSymTab(void)
{
  Object *obj;
  Object *param;

  symtab = (SymTab *)malloc(sizeof(SymTab));
  symtab->globalObjectList = NULL;

  obj = createFunctionObject("READC");
  obj->funcAttrs->returnType = makeCharType();
  addObject(&(symtab->globalObjectList), obj);

  obj = createFunctionObject("READI");
  obj->funcAttrs->returnType = makeIntType();
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITEI");
  param = createParameterObject("i", PARAM_VALUE, obj);
  param->paramAttrs->type = makeIntType();
  addObject(&(obj->procAttrs->paramList), param);
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITEC");
  param = createParameterObject("ch", PARAM_VALUE, obj);
  param->paramAttrs->type = makeCharType();
  addObject(&(obj->procAttrs->paramList), param);
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITELN");
  addObject(&(symtab->globalObjectList), obj);

  intType = makeIntType();
  charType = makeCharType();
}

void cleanSymTab(void)
{
  freeObject(symtab->program);
  freeObjectList(symtab->globalObjectList);
  free(symtab);
  freeType(intType);
  freeType(charType);
}

void enterBlock(Scope *scope)
{
  symtab->currentScope = scope;
}

void exitBlock(void)
{
  symtab->currentScope = symtab->currentScope->outer;
}

void declareObject(Object *obj)
{
  if (obj->kind == OBJ_PARAMETER)
  {
    Object *owner = symtab->currentScope->owner;
    switch (owner->kind)
    {
    case OBJ_FUNCTION:
      addObject(&(owner->funcAttrs->paramList), obj);
      break;
    case OBJ_PROCEDURE:
      addObject(&(owner->procAttrs->paramList), obj);
      break;
    default:
      break;
    }
  }

  addObject(&(symtab->currentScope->objList), obj);
}
