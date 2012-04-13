/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "../dsql/DsqlCompilerScratch.h"
#include "../dsql/DdlNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/jrd.h"
#include "../jrd/blr.h"
#include "../jrd/RecordSourceNodes.h"
#include "../dsql/node.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/pass1_proto.h"

using namespace Firebird;
using namespace Dsql;
using namespace Jrd;


// Write out field data type.
// Taking special care to declare international text.
void DsqlCompilerScratch::putDtype(const dsql_fld* field, bool useSubType)
{
#ifdef DEV_BUILD
	// Check if the field describes a known datatype

	if (field->fld_dtype > FB_NELEM(blr_dtypes) || !blr_dtypes[field->fld_dtype])
	{
		SCHAR buffer[100];

		sprintf(buffer, "Invalid dtype %d in BlockNode::putDtype", field->fld_dtype);
		ERRD_bugcheck(buffer);
	}
#endif

	if (field->fld_not_nullable)
		appendUChar(blr_not_nullable);

	if (field->fld_type_of_name.hasData())
	{
		if (field->fld_type_of_table.hasData())
		{
			if (field->fld_explicit_collation)
			{
				appendUChar(blr_column_name2);
				appendUChar(field->fld_full_domain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(field->fld_type_of_table.c_str());
				appendMetaString(field->fld_type_of_name.c_str());
				appendUShort(field->fld_ttype);
			}
			else
			{
				appendUChar(blr_column_name);
				appendUChar(field->fld_full_domain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(field->fld_type_of_table.c_str());
				appendMetaString(field->fld_type_of_name.c_str());
			}
		}
		else
		{
			if (field->fld_explicit_collation)
			{
				appendUChar(blr_domain_name2);
				appendUChar(field->fld_full_domain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(field->fld_type_of_name.c_str());
				appendUShort(field->fld_ttype);
			}
			else
			{
				appendUChar(blr_domain_name);
				appendUChar(field->fld_full_domain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(field->fld_type_of_name.c_str());
			}
		}

		return;
	}

	switch (field->fld_dtype)
	{
		case dtype_cstring:
		case dtype_text:
		case dtype_varying:
		case dtype_blob:
			if (!useSubType)
				appendUChar(blr_dtypes[field->fld_dtype]);
			else if (field->fld_dtype == dtype_varying)
			{
				appendUChar(blr_varying2);
				appendUShort(field->fld_ttype);
			}
			else if (field->fld_dtype == dtype_cstring)
			{
				appendUChar(blr_cstring2);
				appendUShort(field->fld_ttype);
			}
			else if (field->fld_dtype == dtype_blob)
			{
				appendUChar(blr_blob2);
				appendUShort(field->fld_sub_type);
				appendUShort(field->fld_ttype);
			}
			else
			{
				appendUChar(blr_text2);
				appendUShort(field->fld_ttype);
			}

			if (field->fld_dtype == dtype_varying)
				appendUShort(field->fld_length - sizeof(USHORT));
			else if (field->fld_dtype != dtype_blob)
				appendUShort(field->fld_length);
			break;

		default:
			appendUChar(blr_dtypes[field->fld_dtype]);
			if (DTYPE_IS_EXACT(field->fld_dtype) || (dtype_quad == field->fld_dtype))
				appendUChar(field->fld_scale);
			break;
	}
}

void DsqlCompilerScratch::putType(const TypeClause& type, bool useSubType)
{
#ifdef DEV_BUILD
	// Check if the field describes a known datatype
	if (type.type > FB_NELEM(blr_dtypes) || !blr_dtypes[type.type])
	{
		SCHAR buffer[100];

		sprintf(buffer, "Invalid dtype %d in put_dtype", type.type);
		ERRD_bugcheck(buffer);
	}
#endif

	if (type.notNull)
		appendUChar(blr_not_nullable);

	if (type.typeOfName.hasData())
	{
		if (type.typeOfTable.hasData())
		{
			if (type.collateSpecified)
			{
				appendUChar(blr_column_name2);
				appendUChar(type.fullDomain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(type.typeOfTable.c_str());
				appendMetaString(type.typeOfName.c_str());
				appendUShort(type.textType);
			}
			else
			{
				appendUChar(blr_column_name);
				appendUChar(type.fullDomain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(type.typeOfTable.c_str());
				appendMetaString(type.typeOfName.c_str());
			}
		}
		else
		{
			if (type.collateSpecified)
			{
				appendUChar(blr_domain_name2);
				appendUChar(type.fullDomain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(type.typeOfName.c_str());
				appendUShort(type.textType);
			}
			else
			{
				appendUChar(blr_domain_name);
				appendUChar(type.fullDomain ? blr_domain_full : blr_domain_type_of);
				appendMetaString(type.typeOfName.c_str());
			}
		}

		return;
	}

	switch (type.type)
	{
		case dtype_cstring:
		case dtype_text:
		case dtype_varying:
		case dtype_blob:
			if (!useSubType)
				appendUChar(blr_dtypes[type.type]);
			else if (type.type == dtype_varying)
			{
				appendUChar(blr_varying2);
				appendUShort(type.textType);
			}
			else if (type.type == dtype_cstring)
			{
				appendUChar(blr_cstring2);
				appendUShort(type.textType);
			}
			else if (type.type == dtype_blob)
			{
				appendUChar(blr_blob2);
				appendUShort(type.subType);
				appendUShort(type.textType);
			}
			else
			{
				appendUChar(blr_text2);
				appendUShort(type.textType);
			}

			if (type.type == dtype_varying)
				appendUShort(type.length - sizeof(USHORT));
			else if (type.type != dtype_blob)
				appendUShort(type.length);
			break;

		default:
			appendUChar(blr_dtypes[type.type]);
			if (DTYPE_IS_EXACT(type.type) || dtype_quad == type.type)
				appendUChar(type.scale);
			break;
	}
}

// Emit dyn for the local variables declared in a procedure or trigger.
void DsqlCompilerScratch::putLocalVariables(CompoundStmtNode* parameters, USHORT locals)
{
	if (!parameters)
		return;

	NestConst<StmtNode>* ptr = parameters->statements.begin();

	for (const NestConst<StmtNode>* const end = parameters->statements.end(); ptr != end; ++ptr)
	{
		StmtNode* parameter = *ptr;

		putDebugSrcInfo(parameter->line, parameter->column);

		const DeclareVariableNode* varNode;

		if ((varNode = parameter->as<DeclareVariableNode>()))
		{
			dsql_fld* field = varNode->dsqlDef->legacyField;
			const NestConst<StmtNode>* rest = ptr;

			while (++rest != end)
			{
				const DeclareVariableNode* varNode2;

				if ((varNode2 = (*rest)->as<DeclareVariableNode>()))
				{
					dsql_fld* rest_field = varNode2->dsqlDef->legacyField;

					if (field->fld_name == rest_field->fld_name)
					{
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
								  Arg::Gds(isc_dsql_duplicate_spec) << Arg::Str(field->fld_name));
					}
				}
			}

			dsql_var* variable = makeVariable(field, field->fld_name.c_str(), dsql_var::TYPE_LOCAL,
				0, 0, locals);

			putLocalVariable(variable, varNode, varNode->dsqlDef->collate);

			// Some field attributes are calculated inside putLocalVariable(), so we reinitialize
			// the descriptor.
			MAKE_desc_from_field(&variable->desc, field);

			++locals;
		}
		else if (StmtNode::is<DeclareCursorNode>(parameter) ||
			StmtNode::is<DeclareSubProcNode>(parameter) ||
			StmtNode::is<DeclareSubFuncNode>(parameter))
		{
			parameter->dsqlPass(this);
			parameter->genBlr(this);
		}
		else
			fb_assert(false);
	}
}

// Write out local variable field data type.
void DsqlCompilerScratch::putLocalVariable(dsql_var* variable, const DeclareVariableNode* hostParam,
	const MetaName& collationName)
{
	dsql_fld* field = variable->field;

	appendUChar(blr_dcl_variable);
	appendUShort(variable->number);
	DDL_resolve_intl_type(this, field, collationName);

	//const USHORT dtype = field->fld_dtype;

	putDtype(field, true);
	//field->fld_dtype = dtype;

	// Check for a default value, borrowed from define_domain
	ValueSourceClause* node = hostParam ? hostParam->dsqlDef->defaultClause : NULL;

	if (variable->type == dsql_var::TYPE_INPUT)
	{
		// Assign EXECUTE BLOCK's input parameter to its corresponding internal variable.

		appendUChar(blr_assignment);

		appendUChar(blr_parameter2);
		appendUChar(variable->msgNumber);
		appendUShort(variable->msgItem);
		appendUShort(variable->msgItem + 1);

		appendUChar(blr_variable);
		appendUShort(variable->number);
	}
	else if (node || (!field->fld_full_domain && !field->fld_not_nullable))
	{
		appendUChar(blr_assignment);

		if (node)
		{
			PsqlChanger psqlChanger(this, false);
			GEN_expr(this, PASS1_node(this, node->value));
		}
		else
			appendUChar(blr_null);	// Initialize variable to NULL

		appendUChar(blr_variable);
		appendUShort(variable->number);
	}
	else
	{
		appendUChar(blr_init_variable);
		appendUShort(variable->number);
	}

	if (variable->name.hasData())	// Not a function return value
		putDebugVariable(variable->number, variable->name);

	++hiddenVarsNumber;
}

// Make a variable.
dsql_var* DsqlCompilerScratch::makeVariable(dsql_fld* field, const char* name,
	const dsql_var::Type type, USHORT msgNumber, USHORT itemNumber, USHORT localNumber)
{
	DEV_BLKCHK(field, dsql_type_fld);

	thread_db* tdbb = JRD_get_thread_data();

	dsql_var* dsqlVar = FB_NEW(*tdbb->getDefaultPool()) dsql_var(*tdbb->getDefaultPool());
	dsqlVar->type = type;
	dsqlVar->msgNumber = msgNumber;
	dsqlVar->msgItem = itemNumber;
	dsqlVar->number = localNumber;
	dsqlVar->field = field;
	dsqlVar->name = name;

	if (field)
		MAKE_desc_from_field(&dsqlVar->desc, field);

	if (type == dsql_var::TYPE_HIDDEN)
		hiddenVariables.push(dsqlVar);
	else
	{
		variables.push(dsqlVar);

		if (type == dsql_var::TYPE_OUTPUT)
			outputVariables.push(dsqlVar);
	}

	return dsqlVar;
}

// Try to resolve variable name against parameters and local variables.
dsql_var* DsqlCompilerScratch::resolveVariable(const MetaName& varName)
{
	for (dsql_var* const* i = variables.begin(); i != variables.end(); ++i)
	{
		const dsql_var* variable = *i;

		if (variable->name == varName)
			return *i;
	}

	return NULL;
}

// Generate BLR for a return.
void DsqlCompilerScratch::genReturn(bool eosFlag)
{
	const bool hasEos = !(flags & (FLAG_TRIGGER | FLAG_FUNCTION));

	if (hasEos && !eosFlag)
		appendUChar(blr_begin);

	appendUChar(blr_send);
	appendUChar(1);
	appendUChar(blr_begin);

	for (Array<dsql_var*>::const_iterator i = outputVariables.begin(); i != outputVariables.end(); ++i)
	{
		const dsql_var* variable = *i;
		appendUChar(blr_assignment);
		appendUChar(blr_variable);
		appendUShort(variable->number);
		appendUChar(blr_parameter2);
		appendUChar(variable->msgNumber);
		appendUShort(variable->msgItem);
		appendUShort(variable->msgItem + 1);
	}

	if (hasEos)
	{
		appendUChar(blr_assignment);
		appendUChar(blr_literal);
		appendUChar(blr_short);
		appendUChar(0);
		appendUShort((eosFlag ? 0 : 1));
		appendUChar(blr_parameter);
		appendUChar(1);
		appendUShort(USHORT(2 * outputVariables.getCount()));
	}

	appendUChar(blr_end);

	if (hasEos && !eosFlag)
	{
		appendUChar(blr_stall);
		appendUChar(blr_end);
	}
}

void DsqlCompilerScratch::genParameters(Array<ParameterClause>& parameters,
	Array<ParameterClause>& returns)
{
	if (parameters.hasData())
	{
		fb_assert(parameters.getCount() < MAX_USHORT / 2);
		appendUChar(blr_message);
		appendUChar(0);
		appendUShort(2 * parameters.getCount());

		for (size_t i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause& parameter = parameters[i];
			putDebugArgument(fb_dbg_arg_input, i, parameter.name.c_str());
			putType(parameter, true);

			// Add slot for null flag (parameter2).
			appendUChar(blr_short);
			appendUChar(0);

			makeVariable(parameter.legacyField, parameter.name.c_str(),
				dsql_var::TYPE_INPUT, 0, (USHORT) (2 * i), 0);
		}
	}

	fb_assert(returns.getCount() < MAX_USHORT / 2);
	appendUChar(blr_message);
	appendUChar(1);
	appendUShort(2 * returns.getCount() + 1);

	if (returns.hasData())
	{
		for (size_t i = 0; i < returns.getCount(); ++i)
		{
			ParameterClause& parameter = returns[i];
			putDebugArgument(fb_dbg_arg_output, i, parameter.name.c_str());
			putType(parameter, true);

			// Add slot for null flag (parameter2).
			appendUChar(blr_short);
			appendUChar(0);

			makeVariable(parameter.legacyField, parameter.name.c_str(),
				dsql_var::TYPE_OUTPUT, 1, (USHORT) (2 * i), i);
		}
	}

	// Add slot for EOS.
	appendUChar(blr_short);
	appendUChar(0);
}

void DsqlCompilerScratch::addCTEs(WithClause* withClause)
{
	if (ctes.getCount())
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // WITH clause can't be nested
				  Arg::Gds(isc_dsql_cte_nested_with));
	}

	if (withClause->recursive)
		flags |= DsqlCompilerScratch::FLAG_RECURSIVE_CTE;

	const SelectExprNode* const* end = withClause->end();

	for (SelectExprNode* const* cte = withClause->begin(); cte != end; ++cte)
	{
		if (withClause->recursive)
		{
			currCtes.push(*cte);
			PsqlChanger changer(this, false);
			ctes.add(pass1RecursiveCte(*cte));
			currCtes.pop();

			// Add CTE name into CTE aliases stack. It allows later to search for
			// aliases of given CTE.
			addCTEAlias((*cte)->alias);
		}
		else
			ctes.add(*cte);
	}
}

SelectExprNode* DsqlCompilerScratch::findCTE(const MetaName& name)
{
	for (size_t i = 0; i < ctes.getCount(); ++i)
	{
		SelectExprNode* cte = ctes[i];

		if (cte->alias == name.c_str())
			return cte;
	}

	return NULL;
}

void DsqlCompilerScratch::clearCTEs()
{
	flags &= ~DsqlCompilerScratch::FLAG_RECURSIVE_CTE;
	ctes.clear();
	cteAliases.clear();
}

void DsqlCompilerScratch::checkUnusedCTEs() const
{
	for (size_t i = 0; i < ctes.getCount(); ++i)
	{
		const SelectExprNode* cte = ctes[i];

		if (!(cte->dsqlFlags & RecordSourceNode::DFLAG_DT_CTE_USED))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_cte_not_used) << cte->alias);
		}
	}
}

// Process derived table which can be recursive CTE.
// If it is non-recursive return input node unchanged.
// If it is recursive return new derived table which is an union of union of anchor (non-recursive)
// queries and union of recursive queries. Check recursive queries to satisfy various criterias.
// Note that our parser is right-to-left therefore nested list linked as first node in parent list
// and second node is always query spec.
// For example, if we have 4 CTE's where first two is non-recursive and last two is recursive:
//
//				list							  union
//			  [0]	[1]						   [0]		[1]
//			list	cte3		===>		anchor		recursive
//		  [0]	[1]						 [0]	[1]		[0]		[1]
//		list	cte3					cte1	cte2	cte3	cte4
//	  [0]	[1]
//	cte1	cte2
//
// Also, we should not change layout of original parse tree to allow it to be parsed again if
// needed. Therefore recursive part is built using newly allocated list nodes.
SelectExprNode* DsqlCompilerScratch::pass1RecursiveCte(SelectExprNode* input)
{
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	dsql_nod* query = input->querySpec;
	UnionSourceNode* unionQuery = ExprNode::as<UnionSourceNode>(query);

	if (!unionQuery && pass1RseIsRecursive(query))
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive CTE (%s) must be an UNION
				  Arg::Gds(isc_dsql_cte_not_a_union) << input->alias);
	}

	// split queries list on two parts: anchor and recursive
	dsql_nod* anchorRse = NULL;
	dsql_nod* recursiveRse = NULL;
	dsql_nod* qry = query;

	UnionSourceNode* newQry = FB_NEW(pool) UnionSourceNode(pool);
	dsql_nod* newQryNod = MAKE_class_node(newQry);
	newQry->dsqlClauses = MAKE_node(Dsql::nod_list, 2);

	if (unionQuery)
	{
		newQry->dsqlAll = unionQuery->dsqlAll;
		newQry->recursive = unionQuery->recursive;
	}

	while (true)
	{
		dsql_nod* rse = NULL;

		if (unionQuery)
			rse = unionQuery->dsqlClauses->nod_arg[1];
		else
			rse = qry;

		dsql_nod* newRseNod = pass1RseIsRecursive(rse);

		if (newRseNod) // rse is recursive
		{
			RseNode* newRse = ExprNode::as<RseNode>(newRseNod);

			if (anchorRse)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// CTE '%s' defined non-recursive member after recursive
					Arg::Gds(isc_dsql_cte_nonrecurs_after_recurs) << input->alias);
			}

			if (newRse->dsqlDistinct)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// Recursive member of CTE '%s' has %s clause
					Arg::Gds(isc_dsql_cte_wrong_clause) << input->alias <<
														   Arg::Str("DISTINCT"));
			}

			if (newRse->dsqlGroup)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// Recursive member of CTE '%s' has %s clause
					Arg::Gds(isc_dsql_cte_wrong_clause) << input->alias <<
														   Arg::Str("GROUP BY"));
			}

			if (newRse->dsqlHaving)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// Recursive member of CTE '%s' has %s clause
					Arg::Gds(isc_dsql_cte_wrong_clause) << input->alias <<
														   Arg::Str("HAVING"));
			}
			// hvlad: we need also forbid any aggregate function here
			// but for now i have no idea how to do it simple

			if (!newQry->dsqlAll)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// Recursive members of CTE (%s) must be linked with another members via UNION ALL
					Arg::Gds(isc_dsql_cte_union_all) << input->alias);
			}

			if (!recursiveRse)
				recursiveRse = newQryNod;

			newRse->dsqlFlags |= RecordSourceNode::DFLAG_RECURSIVE;

			if (unionQuery)
				newQry->dsqlClauses->nod_arg[1] = newRseNod;
			else
				newQry->dsqlClauses->nod_arg[0] = newRseNod;
		}
		else
		{
			if (unionQuery)
				newQry->dsqlClauses->nod_arg[1] = rse;
			else
				newQry->dsqlClauses->nod_arg[0] = rse;

			if (!anchorRse)
			{
				if (unionQuery)
					anchorRse = newQryNod;
				else
					anchorRse = rse;
			}
		}

		if (!unionQuery)
			break;

		qry = unionQuery->dsqlClauses->nod_arg[0];
		unionQuery = ExprNode::as<UnionSourceNode>(qry);

		if (unionQuery)
		{
			UnionSourceNode* newUnion = FB_NEW(pool) UnionSourceNode(pool);
			newUnion->dsqlClauses = MAKE_node(Dsql::nod_list, 2);
			newUnion->dsqlAll = unionQuery->dsqlAll;
			newUnion->recursive = unionQuery->recursive;

			newQry->dsqlClauses->nod_arg[0] = MAKE_class_node(newUnion);
			newQryNod = newQry->dsqlClauses->nod_arg[0];
			newQry = newUnion;
		}
	}

	if (!recursiveRse)
		return input;

	if (!anchorRse)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
			// Non-recursive member is missing in CTE '%s'
			Arg::Gds(isc_dsql_cte_miss_nonrecursive) << input->alias);
	}

	UnionSourceNode* qry2 = ExprNode::as<UnionSourceNode>(recursiveRse);
	UnionSourceNode* list = NULL;

	while (qry2->dsqlClauses->nod_arg[0] != anchorRse)
	{
		list = qry2;
		qry2 = ExprNode::as<UnionSourceNode>(qry2->dsqlClauses->nod_arg[0]);
	}

	qry2->dsqlClauses->nod_arg[0] = NULL;

	if (list)
		list->dsqlClauses->nod_arg[0] = qry2->dsqlClauses->nod_arg[1];
	else
		recursiveRse = qry2->dsqlClauses->nod_arg[1];

	UnionSourceNode* unionNode = FB_NEW(pool) UnionSourceNode(pool);
	unionNode->dsqlAll = true;
	unionNode->recursive = true;
	unionNode->dsqlClauses = MAKE_node(Dsql::nod_list, 2);
	unionNode->dsqlClauses->nod_arg[0] = anchorRse;
	unionNode->dsqlClauses->nod_arg[1] = recursiveRse;

	SelectExprNode* select = FB_NEW(getPool()) SelectExprNode(getPool());
	select->querySpec = MAKE_class_node(unionNode);

	select->alias = input->alias;
	select->columns = input->columns;

	return select;
}

// Check if rse is recursive. If recursive reference is a table in the FROM list remove it.
// If recursive reference is a part of join add join boolean (returned by pass1JoinIsRecursive)
// to the WHERE clause. Punt if more than one recursive reference is found.
dsql_nod* DsqlCompilerScratch::pass1RseIsRecursive(dsql_nod* inputNod)
{
	RseNode* input = ExprNode::as<RseNode>(inputNod);
	fb_assert(input);

	RseNode* result = FB_NEW(getPool()) RseNode(getPool());
	result->dsqlFirst = input->dsqlFirst;
	result->dsqlSkip = input->dsqlSkip;
	result->dsqlDistinct = input->dsqlDistinct;
	result->dsqlSelectList = input->dsqlSelectList;
	result->dsqlWhere = input->dsqlWhere;
	result->dsqlGroup = input->dsqlGroup;
	result->dsqlHaving = input->dsqlHaving;
	result->dsqlPlan = input->dsqlPlan;

	dsql_nod* srcTables = input->dsqlFrom;
	dsql_nod* dstTables = MAKE_node(Dsql::nod_list, srcTables->nod_count);
	result->dsqlFrom = dstTables;

	dsql_nod** pDstTable = dstTables->nod_arg;
	dsql_nod** pSrcTable = srcTables->nod_arg;
	dsql_nod** end = srcTables->nod_arg + srcTables->nod_count;
	bool found = false;

	for (dsql_nod** prev = pDstTable; pSrcTable < end; ++pSrcTable, ++pDstTable)
	{
		*prev++ = *pDstTable = *pSrcTable;

		switch ((*pDstTable)->nod_type)
		{
			case Dsql::nod_class_exprnode:
			{
				RseNode* rseNode = ExprNode::as<RseNode>(*pDstTable);

				if (rseNode)
				{
					fb_assert(rseNode->dsqlExplicitJoin);

					RseNode* dstRse = rseNode->clone();

					*pDstTable = MAKE_node(Dsql::nod_class_exprnode, 1);
					(*pDstTable)->nod_arg[0] = reinterpret_cast<dsql_nod*>(dstRse);

					dsql_nod* joinBool = pass1JoinIsRecursive(*pDstTable);

					if (joinBool)
					{
						if (found)
						{
							ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
									  // Recursive member of CTE can't reference itself more than once
									  Arg::Gds(isc_dsql_cte_mult_references));
						}

						found = true;

						result->dsqlWhere = PASS1_compose(result->dsqlWhere, joinBool, blr_and);
					}

					break;
				}
				else if (ExprNode::is<ProcedureSourceNode>(*pDstTable) ||
					ExprNode::is<RelationSourceNode>(*pDstTable))
				{
					if (pass1RelProcIsRecursive(*pDstTable))
					{
						if (found)
						{
							ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
									  // Recursive member of CTE can't reference itself more than once
									  Arg::Gds(isc_dsql_cte_mult_references));
						}
						found = true;

						--prev;
						--dstTables->nod_count;
					}
					break;
				}
				else if (ExprNode::is<SelectExprNode>(*pDstTable))
					break;

				// fall into
			}

			default:
				fb_assert(false);
		}
	}

	if (found)
	{
		dsql_nod* resultNod = MAKE_node(Dsql::nod_class_exprnode, 1);
		resultNod->nod_arg[0] = reinterpret_cast<dsql_nod*>(result);
		return resultNod;
	}

	return NULL;
}

// Check if table reference is recursive i.e. its name is equal to the name of current processing CTE.
bool DsqlCompilerScratch::pass1RelProcIsRecursive(dsql_nod* input)
{
	MetaName relName;
	string relAlias;
	ProcedureSourceNode* procNode;
	RelationSourceNode* relNode;

	if ((procNode = ExprNode::as<ProcedureSourceNode>(input)))
	{
		relName = procNode->dsqlName.identifier;
		relAlias = procNode->alias;
	}
	else if ((relNode = ExprNode::as<RelationSourceNode>(input)))
	{
		relName = relNode->dsqlName;
		relAlias = relNode->alias;
	}
	else
		return false;

	fb_assert(currCtes.hasData());
	const SelectExprNode* currCte = currCtes.object();
	const bool recursive = currCte->alias == relName.c_str();

	if (recursive)
		addCTEAlias(relAlias.hasData() ? relAlias.c_str() : relName.c_str());

	return recursive;
}

// Check if join have recursive members. If found remove this member from join and return its
// boolean (to be added into WHERE clause).
// We must remove member only if it is a table reference. Punt if recursive reference is found in
// outer join or more than one recursive reference is found
dsql_nod* DsqlCompilerScratch::pass1JoinIsRecursive(dsql_nod*& inputNod)
{
	RseNode* input = ExprNode::as<RseNode>(inputNod);
	fb_assert(input);

	const UCHAR join_type = input->rse_jointype;
	bool remove = false;

	bool leftRecursive = false;
	dsql_nod* leftBool = NULL;
	dsql_nod** join_table = &input->dsqlFrom->nod_arg[0];
	RseNode* joinRse;

	if ((joinRse = ExprNode::as<RseNode>(*join_table)) && joinRse->dsqlExplicitJoin)
	{
		leftBool = pass1JoinIsRecursive(*join_table);
		leftRecursive = (leftBool != NULL);
	}
	else
	{
		leftBool = input->dsqlWhere;
		leftRecursive = pass1RelProcIsRecursive(*join_table);

		if (leftRecursive)
			remove = true;
	}

	if (leftRecursive && join_type != blr_inner)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't be member of an outer join
				  Arg::Gds(isc_dsql_cte_outer_join));
	}

	bool rightRecursive = false;
	dsql_nod* rightBool = NULL;

	join_table = &input->dsqlFrom->nod_arg[1];

	if ((joinRse = ExprNode::as<RseNode>(*join_table)) && joinRse->dsqlExplicitJoin)
	{
		rightBool = pass1JoinIsRecursive(*join_table);
		rightRecursive = (rightBool != NULL);
	}
	else
	{
		rightBool = input->dsqlWhere;
		rightRecursive = pass1RelProcIsRecursive(*join_table);

		if (rightRecursive)
			remove = true;
	}

	if (rightRecursive && join_type != blr_inner)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't be member of an outer join
				  Arg::Gds(isc_dsql_cte_outer_join));
	}

	if (leftRecursive && rightRecursive)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't reference itself more than once
				  Arg::Gds(isc_dsql_cte_mult_references));
	}

	if (leftRecursive)
	{
		if (remove)
			inputNod = input->dsqlFrom->nod_arg[1];

		return leftBool;
	}

	if (rightRecursive)
	{
		if (remove)
			inputNod = input->dsqlFrom->nod_arg[0];

		return rightBool;
	}

	return NULL;
}