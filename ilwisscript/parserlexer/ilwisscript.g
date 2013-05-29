grammar ilwisscript;

options {
  language = C;

}

@preincludes{
#include <QString>
#include <QStringList>
#include <QHash>
#include "ilwis.h"
#include "geometries.h"
#include "astnode.h"
#include "idnode.h"
#include "scriptnode.h"
#include "operationnode.h"
#include "expressionnode.h"
#include "assignmentnode.h"
#include "scriptlinenode.h"
#include "parametersNode.h"
#include "rangedefinitionnode.h"
#include "termnode.h"
#include "variableNode.h"
#include "returnnode.h"
#include "functionnode.h"
#include "multiplicationnode.h"
#include "addnode.h"
#include "relationnode.h"
#include "commandnode.h"
#include "whilenode.h"
#include "breaknode.h"
#include "formatnode.h"
#include "valuerangenode.h"
#include "dompartnode.h"
#include "modifierpart.h"
#include "functionstatementnode.h"
}


script returns [ ASTNode *node]
@init{
	node = new ScriptNode();
}
	:	( scriptLine  {node->addChild($scriptLine.node); }  )+
	;
	
scriptLine returns [ ASTNode *node] 
@init{
	node =  new ScriptLineNode();
}
	:	statement				 { node->addChild($statement.node); } 	';'
	|	variable 				 { node->addChild($variable.node); } ';'
	|	constant				 { node->addChild($constant.node); } ';'
	|	function 				 { node->addChild($function.node); } ';'
	| 	EOL
	;
	
variable returns [ VariableNode *node]
@init{
	node = new VariableNode(false);
}
	:	'declare'id1 =  ID			{node->addChild(new IDNode((char *)($id1.text->chars)));}
		(
		',' 
		id2=ID					{node->addChild(new IDNode((char *)($id2.text->chars)));}
		)* 
		':'
		type 					{ node->setType($type.type); }				
		(':=' 
		expression				{ node->setExpression($expression.node); }
		)? 
	;	
	
constant returns [ VariableNode *node]
@init{
	node = new VariableNode(true);
}
	:	'constant' id1 = ID 				{ node->addChild(new IDNode((char *)($id1.text->chars)));}
		':' 
		type 						{ node->setType($type.type); }
		':=' 
		expression					{ node->setExpression($expression.node); } 
	;
	
type returns [ quint64 type]
	:	'Integer'					{ type= itINT32; }
	|	'Boolean'					{ type =  itBOOL; }
	|	'String'					{ type=  itSTRING;}
	|	'Real'						{ type= itDOUBLE; }
	;	
	
statement returns [ ASTNode *node]
	:	assignmentStatement				{ node = $assignmentStatement.node; }
	|	ifStatement					{ node = $ifStatement.node; }
	|	whileStatement					{ node = $whileStatement.node; }
	|	functionStatement				{ node = $functionStatement.node; }
	|	commandStatement				{ node = $commandStatement.node; }
	|	formatModifier					{ node = $formatModifier.node; }
	;
	
assignmentStatement returns [ AssignmentNode *node]
@init{
	node= new AssignmentNode();
}
	:	ID 						{ node->setResult(new IDNode((char *)($ID.text->chars))); }
		(typeModifierPart 				{ node->setTypeModifierPart($typeModifierPart.node); }
		)?
		(':=' | '=')					{ node->setDefintion(true); } 
		expression 					{ node->setExpression($expression.node);}
	;	

actualParameters returns [ ParametersNode *node]
@init{
	node = new ParametersNode();
}
	:	expr1 = expression 				{ node->addChild($expr1.node); }
		(
		',' 
		expr2 = expression				{ node->addChild($expr2.node); }
		)*
	;
	
function returns [ FunctionNode *node]
	:	'function' ID					{ node = new FunctionNode((char *)($ID.text->chars));}
		'(' (var1=variable 				{ node->addParameter(var1); }
		)?
		(',' var2=variable 				{ node->addParameter(var2); }
		)*
		')' EOL			
		'begin' EOL
		(statement 					{ node->addChild($statement.node); }
		EOL)*
		( returnStatement 				{ node->setReturn($returnStatement.node); }
		)?
		'endfunction'
	;		
	
term returns [ TermNode *node]
@init{
	node = new TermNode();
}
	:	'(' expression ')'				{ node->setExpression($expression.node); }	
	|	INT						{ node->setNumerical((char *)($INT.text->chars)); }
	|	FLOAT						{ node->setNumerical((char *)($FLOAT.text->chars)); }
	|	STRING						{ node->setString((char *)($STRING.text->chars)); }
	|	id1 = ID '(' 					{ node->setId(new IDNode((char *)($id1.text->chars))); }
		(actualParameters				{   node->setParameters($actualParameters.node); }
		)? ')'						
	|	id2 = ID (id3=rangeDef)+			{ node->setId(new IDNode((char *)($id2.text->chars)));node->addRange($id3.node);}
	|	id2 = ID					{ node->setId(new IDNode((char *)($id2.text->chars)));}
	;

rangeDef returns [ RangeDefinitionNode *node]
@init{
	node = new RangeDefinitionNode("rangedefinitionnode");
}
	: '[' id1=INT id2=INT ',' id3=INT id4=INT ']'		{ node->setRangeType("box"); node->setBounds((char *)($id1.text->chars),
										(char *)($id2.text->chars),
										(char *)($id3.text->chars),
										(char *)($id4.text->chars)); }
	| '[' ID ']'						{ node->setRangeType("var"); node->setVariable((char *)($ID.text->chars));}
	| '[' INT ']'						{ node->setRangeType("index"); node->setVariable((char *)($INT.text->chars));}
	;

negation  returns [ TermNode *node]
@init{
	bool negated = false;
}
	:	('not' 					{ negated = !negated; }
		)*
		term					{ $term.node->setLogicalNegation(negated); node = $term.node;}
	;

unary  returns [ TermNode *node]
@init{
	bool isnegated = false;
}

	:	('+' 					
		| '-'					{ isnegated = !isnegated; }
		)? 
		negation				{ $negation.node->setNumericalNegation(isnegated); node = $negation.node; }
	;
mult  returns [ MultiplicationNode *node]
@init{
	node = new MultiplicationNode();
	OperationNode::Operators op;
}
	:	un1=unary 				{ node->setLeftTerm(un1); }
		(
		('*' 					{ op = OperationNode::oTIMES; }
		| '/'					{ op = OperationNode::oDIVIDED; }
		| 'mod'					{ op = OperationNode::oMOD; }
		) 
		un2=unary				{ node->addRightTerm(op, un2); }
		)*
	;
	
add  returns [ AddNode *node]
@init{
	node = new AddNode();
	OperationNode::Operators op;
}
	:	mul1 = mult 				{ node->setLeftTerm(mul1); }
		(
		('+'					{ op = OperationNode::oADD; } 
		| 
		'-'					{ op = OperationNode::oSUBSTRACT; }
		) 	
		mul2 = mult				{ node->addRightTerm(op, mul2); }
		)*
	;
	
relation  returns [ RelationNode *node]
@init{
	node = new RelationNode();
	OperationNode::Operators op;
}
	:	add1=add				{ node->setLeftTerm(add1); } 
		(
		(
		'==' 					{ op = OperationNode::oEQ; } 
		| '!=' 					{ op = OperationNode::oNEQ; } 
		| '<' 					{ op = OperationNode::oLESS; } 
		| '<=' 					{ op = OperationNode::oLESSEQ; } 
		| '>=' 					{ op = OperationNode::oGREATEREQ; } 
		| '>'					{ op = OperationNode::oGREATER; } 
		) 
		add2=add				{ node->addRightTerm(op, add2); }
		)*
	;
	
expression returns [ ExpressionNode *node]
@init{
	node = new ExpressionNode();
	OperationNode::Operators op;
}
	:	rel1 = relation				{ node->setLeftTerm(rel1); } 
		(				
		( 'and'  				{ op = OperationNode::oAND; } 
		| 'or'  				{ op = OperationNode::oOR; } 
		| 'xor' 				{ op = OperationNode::oXOR; } 
		) rel2 = relation			{ node->addRightTerm(op, rel2); }
		) *
	;		

commandStatement returns [ CommandNode *node]
@init{
	node = new CommandNode();
	QString flag;
	ExpressionNode *expr;
}
	:	
	id1 = ID					{ node->setCommand((char *)($id1.text->chars)); }						 
	(
	' -' 
	id2 = ID 					{ flag = QString((char *)($id2.text->chars));}
	('='
	expression					{ expr = $expression.node; }
	)? 						{ node->addOption(flag, expr); expr = 0;}
	)*
	;	
	
functionStatement returns [ FunctionStatementNode *node]
	:	ID					{ node = new FunctionStatementNode((char *)($ID.text->chars)); } 
	'(' actualParameters? 				{ node->setParameters($actualParameters.node); }
	')' 
	;	
	
whileStatement returns [ WhileNode *node]
@init{
	node = new WhileNode();
}
	:	'while' expression EOL			{ node->setCondition($expression.node); }
		( (statement|				{ node->addChild($statement.node); }
		breakStatement				{ node->addChild($breakStatement.node); }
		) EOL
		)*
		'endwhile' 
	;
	
breakStatement returns [ BreakNode *node]
@init{
	node = new BreakNode();
}
	:	'break' 'when' expression		{ node->addChild($expression.node); } 
	;		

typeModifierPart returns [ ModifierPart *node]
@init{
}
	:	'{' modifierPart'}'			{ node=$modifierPart.node;}
	;

modifierPart returns [ ModifierPart *node]
@init{
	node = new ModifierPart();
}		
	: formatModifier				{ node->addChild($formatModifier.node);}
	| domPart					{ node->addChild($domPart.node);}
	| formatModifier ';' domPart			{ node->addChild($formatModifier.node);node->addChild($domPart.node);}
	;

formatModifier returns [ FormatNode *node]
@init{
	node = new FormatNode();
}	
	: 'format' '=' ID ':' STRING ':' id1=datatype 	{ node->setDataType($id1.typeName);
							  node->setFormatNameSpace(new IDNode((char *)($ID.text->chars)));
							  node->setFormat((char *)($STRING.text->chars));}	
	| 'format' '=' ID ':' STRING			{ node->setFormatNameSpace(new IDNode((char *)($ID.text->chars)));
							  node->setFormat((char *)($STRING.text->chars));}
	| 'format' '=' STRING				{ node->setFormat((char *)($STRING.text->chars)); }
	;

datatype returns [ QString typeName]
	: 'gridcoverage'				{ typeName = "gridcoverage"; }
	| 'polygoncoverage'				{ typeName = "polygoncoverage"; }
	| 'linecoverage'				{ typeName = "linecoverage";}
	| 'pointcoverage'				{ typeName = "pointcoverage"; }
	| 'table'					{ typeName = "table"; }	
	;
	
domPart returns [ DomPartNode *node]
@init{
	node = new DomPartNode();
}
	:	'dom' '=' ID 			{ node->setDomainId(new IDNode((char *)($ID.text->chars)));}
	|	'dom' '=' valrangePart 		{ node->setValueRange($valrangePart.node);}
	;	

valrangePart returns [ ValueRangeNode *node]
	:	'vr' '=' 
	;	
	
ifStatement returns [ ASTNode *node]
	:	'if' expression 'then' statement+
		('elsif' expression 'then' statement+)*
		('else' statement+)?
		'endif' 
	;
returnStatement returns [ ReturnNode *node]
@init{
	node = new ReturnNode();
}
	:	'return' expression?			{node->addChild($expression.node); } 
	;
		
	
	
// LEXER---------------------------------------------------------------------
//---------------------------------------------------------------------------
ID  :	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_'|'.')*
    ;

INT :	'0'..'9'+
    ;

FLOAT
    :   ('0'..'9')+ '.' ('0'..'9')* EXPONENT?
    |   '.' ('0'..'9')+ EXPONENT?
    |   ('0'..'9')+ EXPONENT
    ;

COMMENT
    :   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    |   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    |	'Rem' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    ;

WS  :   ( ' '
        | '\t'
        | '\r'
        ) {$channel=HIDDEN;}
    ;

STRING
    :  '"' ( ~('"') )* '"'
    ;

fragment
EXPONENT : ('e'|'E') ('+'|'-')? ('0'..'9')+ ;


fragment
ESC_SEQ
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    ;

EOL 	: ('\r' | '\n')+;

