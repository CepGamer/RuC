//
//  codegen.c
//
//  Created by Andrey Terekhov on 3/10/15.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"

extern void error(int err);

int moderef;

#ifdef LLGEN
#include <string.h>
#include <llvm-c\Core.h>
#include <llvm-c\BitWriter.h>

#define LLBOOL 900

#define LLCOMP 1000

typedef union u
{
	int i;
	float f;
} u;

LLVMModuleRef ll_module = NULL;
LLVMBuilderRef ll_builder = NULL;
LLVMValueRef ll_func = NULL, ll_printf, ll_memcpy;
LLVMValueRef print[12];
LLVMBasicBlockRef ll_funend, ll_continue;

int ll_sp = 0, ll_spl = 0;
int regs[10000] = { 0 };
int ll_identab[10000] = { NULL };
int reg_param[100] = { 0 }, ll_stack[100] = { 0 }, ll_labels[100] = { 0 };
LLVMValueRef ll_val_stack[100] = { NULL };
int values_arr[100];
int strings_arr[100];
int level;
char ll_reprtab[100] = { 0 };

int ll_repr_cpy(int repr) {
	int i = 0;
	repr += 2;
	while (ll_reprtab[i] = (char)(reprtab[repr + i]))
		i++;
	return i;
}

LLVMTypeRef ll_type_ref(int type){
	switch (type)
	{
	case LCHAR:
		LLVMInt8Type();
		break;
	case LINT:
		return LLVMInt32Type();
		break;
	case LFLOAT:
		return LLVMFloatType();
		break;
	case LVOID:
		return LLVMVoidType();
		break;
	default:
		return NULL;
		break;
	}
}

LLVMValueRef ll_ret_const(int type, int value) {
	u val;
	switch (type) {
	case LFLOAT:
		val.i = value;
		return LLVMConstReal(LLVMFloatType(), val.f);
		break;
	case LCHAR:
		return LLVMConstInt(LLVMInt8Type(), value, 0);
		break;
	case LLBOOL:
		return LLVMConstInt(LLVMInt1Type(), value, 0);
		break;
	case LINT:
	default:
		return LLVMConstInt(LLVMInt32Type(), value, 0);
		break;
	}
}

LLVMValueRef ll_ret_last_val() {
	LLVMValueRef toRet = NULL;
	if (ll_sp < 2)
		return NULL;
	switch (ll_stack[ll_sp - 2])
	{
	case TIdent:
		toRet = LLVMBuildLoad(ll_builder, ll_identab[ll_stack[ll_sp - 1]], "load");
		LLVMSetAlignment(toRet, ansttype == LCHAR ? 1 : 4);
		break;
	case TConst:
		toRet = ll_ret_const(ansttype, ll_stack[ll_sp - 1]);
		break;
	case LLCOMP:
	case TIdenttoval:
		toRet = (LLVMValueRef)ll_stack[ll_sp - 1];
		break;
	default:
		break;
	}
	ll_sp -= 2;
	return toRet;
}

void ll_fill_tmp(LLVMValueRef *tmp) {
	int tmp_ind = 1;
	do
	switch (ll_stack[ll_sp - 2])
	{
		case TConst:
			tmp[tmp_ind] = ll_ret_const(ansttype, ll_stack[ll_sp - 1]);
			break;
		case TIdent:
			tmp[tmp_ind] = LLVMBuildLoad(ll_builder, ll_identab[ll_stack[ll_sp - 1]], "val");
			LLVMSetAlignment(tmp[tmp_ind], ansttype == LCHAR ? 1 : 4);
			break;
		case TIdenttoval:
		case LLCOMP:
			tmp[tmp_ind] = ll_stack[ll_sp - 1];
			break;
		default:
			break;
	}
	while (ll_sp -= 2, tmp_ind-- > 0);
	ll_stack[ll_sp++] = LLCOMP;
	ll_sp++;
}

void ll_ass_fun(int code) {
	LLVMValueRef val, res, load;
	if ((code >= POSTINC && code <= DEC) ||
		(code >= POSTINCV && code <= DECV) ||
		(code >= POSTINCR && code <= DECR) ||
		(code >= POSTINCRV && code <= DECRV)) {
		LLVMValueRef retval;
		retval = load = LLVMBuildLoad(ll_builder, ll_identab[ll_stack[ll_sp - 1]], "ld");
		LLVMSetAlignment(load, ansttype == LCHAR ? 1 : 4);
		switch (code)
		{
		case POSTINCR:
		case POSTINCRV:
			res = LLVMBuildFAdd(ll_builder, load, ll_ret_const(LFLOAT, 1), "inc");
			break;

		case POSTDECR:
		case POSTDECRV:
			res = LLVMBuildFSub(ll_builder, load, ll_ret_const(LFLOAT, 1), "dec");
			break;

		case INCR:
		case INCRV:
			retval = res = LLVMBuildFAdd(ll_builder, load, ll_ret_const(LFLOAT, 1), "inc");
			break;

		case DECR:
		case DECRV:
			retval = res = LLVMBuildFSub(ll_builder, load, ll_ret_const(LFLOAT, 1), "dec");
			break;

		case POSTINC:
		case POSTINCV:
			res = LLVMBuildAdd(ll_builder, load, ll_ret_const(LINT, 1), "inc");
			break;

		case POSTDEC:
		case POSTDECV:
			res = LLVMBuildSub(ll_builder, load, ll_ret_const(LINT, 1), "dec");
			break;

		case INC:
		case INCV:
			retval = res = LLVMBuildAdd(ll_builder, load, ll_ret_const(LINT, 1), "inc");
			break;

		case DEC:
		case DECV:
			retval = res = LLVMBuildSub(ll_builder, load, ll_ret_const(LINT, 1), "dec");
			break;

		default:
			break;
		}
		LLVMSetAlignment(LLVMBuildStore(ll_builder, res, ll_identab[ll_stack[ll_sp - 1]]), ansttype == LCHAR ? 1 : 4);
		ll_stack[ll_sp - 2] = LLCOMP;
		ll_stack[ll_sp - 1] = retval;

		if ((code >= POSTINCV && code <= DECV) ||
			(code >= POSTINCRV && code <= DECRV))
			ll_sp -= 2;
		return;
	}
	val = ll_ret_last_val();
	if (!level && (code == ASS || code == ASSV)) {
		LLVMSetInitializer(ll_identab[ll_stack[ll_sp - 1]], val);
		ll_stack[ll_sp - 1] = val;
		ll_stack[ll_sp - 2] = LLCOMP;
		if (code == ASSV)
			ll_sp -= 2;
		return;
	}
	else
		LLVMSetAlignment(load = LLVMBuildLoad(ll_builder, ll_identab[ll_stack[ll_sp - 1]], "ld"), ansttype == LCHAR ? 1 : 4);
	switch (code) {
	case ASS:
	case ASSV:
		res = val;
		break;

	case SHLASS:
	case SHLASSV:
		res = LLVMBuildShl(ll_builder, load, val, "ass_shl");
		break;

	case SHRASS:
	case SHRASSV:
		res = LLVMBuildLShr(ll_builder, load, val, "ass_shr");
		break;

	case PLUSASS:
	case PLUSASSV:
		res = LLVMBuildAdd(ll_builder, load, val, "ass_sum");
		break;

	case PLUSASSR:
	case PLUSASSRV:
		res = LLVMBuildFAdd(ll_builder, load, val, "ass_sum");
		break;

	case MINUSASS:
	case MINUSASSV:
		res = LLVMBuildSub(ll_builder, load, val, "ass_sub");
		break;

	case MINUSASSR:
	case MINUSASSRV:
		res = LLVMBuildFSub(ll_builder, load, val, "ass_sub");
		break;

	case MULTASS:
	case MULTASSV:
		res = LLVMBuildMul(ll_builder, load, val, "ass_mult");
		break;
	
	case MULTASSR:
	case MULTASSRV:
		res = LLVMBuildFMul(ll_builder, load, val, "ass_mult");
		break;

	case DIVASS:
	case DIVASSV:
		res = LLVMBuildSDiv(ll_builder, load, val, "ass_div");
		break;
	
	case DIVASSR:
	case DIVASSRV:
		res = LLVMBuildFMul(ll_builder, load, val, "ass_div");
		break;
	
	case REMASS:
	case REMASSV:
		res = LLVMBuildSRem(ll_builder, load, val, "ass_rem");
		break;

	case ANDASS:
	case ANDASSV:
		res = LLVMBuildAnd(ll_builder, load, val, "ass_and");
		break;

	case EXORASS:
	case EXORASSV:
		res = LLVMBuildXor(ll_builder, load, val, "ass_xor");
		break;

	case ORASS:
	case ORASSV:
		res = LLVMBuildOr(ll_builder, load, val, "ass_or");
		break;
	default:
		break;
	}
	LLVMSetAlignment(LLVMBuildStore(ll_builder, res, ll_identab[ll_stack[ll_sp - 1]]), ansttype == LCHAR ? 1 : 4);
	ll_stack[ll_sp - 2] = LLCOMP;
	ll_stack[ll_sp - 1] = res;
	if ((code >= ASSV && code <= DIVASSV) ||
		(code >= PLUSASSRV && code <= DIVASSRV))
		ll_sp -= 2;
	return;
}

void ll_assat(int code)
{
	LLVMValueRef tmp[2] = { NULL, NULL }, val;
	if ((code >= POSTINCAT && code <= DECAT) ||
		(code >= POSTINCATV && code <= DECATV) ||
		(code >= POSTINCATR && code <= DECATR) ||
		(code >= POSTINCATRV && code <= DECATRV)) {
		LLVMValueRef retval = NULL;
		tmp[0] = ll_ret_last_val();
		ll_sp += 2;
		val = LLVMBuildLoad(ll_builder, tmp[0], "ld");
		LLVMSetAlignment(val, ansttype == LCHAR ? 1 : 4);
		switch (code) {
		case INCAT:
		case INCATV:
			retval = val = LLVMBuildAdd(ll_builder, val, ll_ret_const(LINT, 1), "inc");
			break;

		case DECAT:
		case DECATV:
			retval = val = LLVMBuildSub(ll_builder, val, ll_ret_const(LINT, 1), "dec");
			break;

		case POSTINCAT:
		case POSTINCATV:
			val = LLVMBuildAdd(ll_builder, retval, ll_ret_const(LINT, 1), "postinc");
			break;

		case POSTDECAT:
		case POSTDECATV:
			val = LLVMBuildSub(ll_builder, val, ll_ret_const(LINT, 1), "postdec");
			break;

		case INCATR:
		case INCATRV:
			retval = val = LLVMBuildFAdd(ll_builder, val, ll_ret_const(LFLOAT, 1), "inc");
			break;

		case DECATR:
		case DECATRV:
			retval = val = LLVMBuildSub(ll_builder, val, ll_ret_const(LFLOAT, 1), "dec");
			break;

		case POSTINCATR:
		case POSTINCATRV:
			val = LLVMBuildFAdd(ll_builder, retval, ll_ret_const(LFLOAT, 1), "postinc");
			break;

		case POSTDECATR:
		case POSTDECATRV:
			val = LLVMBuildFSub(ll_builder, val, ll_ret_const(LFLOAT, 1), "postdec");
			break;
		default:
			break;
		}
		LLVMSetAlignment(LLVMBuildStore(ll_builder, val, tmp[0]), ansttype == LCHAR ? 1 : 4);
		ll_stack[ll_sp - 2] = LLCOMP;
		ll_stack[ll_sp - 1] = retval;
		if ((code >= POSTINCATV && code <= DECATV) ||
			(code >= POSTINCATRV && code <= DECRV))
			ll_sp -= 2;
		return;
	}
	ll_fill_tmp(tmp);
	if (code != ASSAT && code != ASSATV)
		LLVMSetAlignment(val = LLVMBuildLoad(ll_builder, tmp[0], "ld"), ansttype == LCHAR ? 1 : 4);
	switch (code) {
	case ASSAT:
	case ASSATV:
		val = tmp[1];
		break;

	case REMASSAT:
	case REMASSATV:
		val = LLVMBuildSRem(ll_builder, val, tmp[1], "rem_res");
		break;

	case SHLASSAT:
	case SHLASSATV:
		val = LLVMBuildShl(ll_builder, val, tmp[1], "shl_res");
		break;

	case SHRASSAT:
	case SHRASSATV:
		val = LLVMBuildLShr(ll_builder, val, tmp[1], "shr_res");
		break;

	case ANDASSAT:
	case ANDASSATV:
		val = LLVMBuildAnd(ll_builder, val, tmp[1], "and_res");
		break;

	case EXORASSAT:
	case EXORASSATV:
		val = LLVMBuildXor(ll_builder, val, tmp[1], "xor_res");
		break;

	case ORASSAT:
	case ORASSATV:
		val = LLVMBuildOr(ll_builder, val, tmp[1], "or_res");
		break;

	case PLUSASSAT:
	case PLUSASSATV:
		val = LLVMBuildAdd(ll_builder, val, tmp[1], "sum_res");
		break;

	case PLUSASSATR:
	case PLUSASSATRV:
		val = LLVMBuildFAdd(ll_builder, val, tmp[1], "sum_res");
		break;

	case MINUSASSAT:
	case MINUSASSATV:
		val = LLVMBuildSub(ll_builder, val, tmp[1], "sub_res");
		break;

	case MINUSASSATR:
	case MINUSASSATRV:
		val = LLVMBuildFSub(ll_builder, val, tmp[1], "sub_res");
		break;

	case MULTASSAT:
	case MULTASSATV:
		val = LLVMBuildMul(ll_builder, val, tmp[1], "mul_res");
		break;

	case MULTASSATR:
	case MULTASSATRV:
		val = LLVMBuildFMul(ll_builder, val, tmp[1], "mul_res");
		break;

	case DIVASSAT:
	case DIVASSATV:
		val = LLVMBuildSDiv(ll_builder, val, tmp[1], "div_res");
		break;

	case DIVASSATR:
	case DIVASSATRV:
		val = LLVMBuildFDiv(ll_builder, val, tmp[1], "div_res");
		break;

	default:
		break;
	}
	LLVMSetAlignment(LLVMBuildStore(ll_builder, val, tmp[0]), ansttype == LCHAR ? 1 : 4);
	ll_stack[ll_sp - 2] = LLCOMP;
	ll_stack[ll_sp - 1] = val;

	if ((code >= PLUSASSATV && code <= DIVASSATV) ||
		(code >= PLUSASSATRV && code <= DIVASSATRV))
		ll_sp -= 2;
}

void ll_binop(int code) {
	LLVMValueRef tmp[] = { NULL, NULL };
	ll_fill_tmp(tmp);
	switch (code){
	case LPLUS:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildAdd(ll_builder, tmp[0], tmp[1], "add");
		else
			ll_stack[ll_sp - 1] = (int)(void*)LLVMConstAdd(tmp[0], tmp[1]);
		break;
	case LPLUSR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFAdd(ll_builder, tmp[0], tmp[1], "add");
		else
			ll_stack[ll_sp - 1] = (int)(void*)LLVMConstFAdd(tmp[0], tmp[1]);
		break;
	case LMINUS:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildSub(ll_builder, tmp[0], tmp[1], "sub");
		else
			ll_stack[ll_sp - 1] = (int)(void*)LLVMConstSub(tmp[0], tmp[1]);
		break;
	case LMINUSR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFSub(ll_builder, tmp[0], tmp[1], "sub");
		else
			ll_stack[ll_sp - 1] = (int)(void*)LLVMConstFSub(tmp[0], tmp[1]);
		break;
	case LREM:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildSRem(ll_builder, tmp[0], tmp[1], "srem");
		else
			ll_stack[ll_sp - 1] = LLVMConstSRem(tmp[0], tmp[1]);
		break;
	case LMULT:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildMul(ll_builder, tmp[0], tmp[1], "mul");
		else
			ll_stack[ll_sp - 1] = LLVMConstMul(tmp[0], tmp[1]);
		break;
	case LMULTR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFMul(ll_builder, tmp[0], tmp[1], "mul");
		else
			ll_stack[ll_sp - 1] = LLVMConstFMul(tmp[0], tmp[1]);
		break;
	case LDIV:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildSDiv(ll_builder, tmp[0], tmp[1], "div");
		else
			ll_stack[ll_sp - 1] = LLVMConstSDiv(tmp[0], tmp[1]);
		break;
	case LDIVR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFDiv(ll_builder, tmp[0], tmp[1], "div");
		else
			ll_stack[ll_sp - 1] = LLVMConstFDiv(tmp[0], tmp[1]);
		break;
	case LSHL:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildShl(ll_builder, tmp[0], tmp[1], "shl");
		else
			ll_stack[ll_sp - 1] = LLVMConstShl(tmp[0], tmp[1]);
		break;
	case LSHR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildLShr(ll_builder, tmp[0], tmp[1], "shr");
		else
			ll_stack[ll_sp - 1] = LLVMConstLShr(tmp[0], tmp[1]);
		break;
	case LAND:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildAnd(ll_builder, tmp[0], tmp[1], "and");
		else
			ll_stack[ll_sp - 1] = LLVMConstAnd(tmp[0], tmp[1]);
		break;
	case LEXOR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildXor(ll_builder, tmp[0], tmp[1], "xor");
		else
			ll_stack[ll_sp - 1] = LLVMConstXor(tmp[0], tmp[1]);
		break;
	case LOR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildOr(ll_builder, tmp[0], tmp[1], "or");
		else
			ll_stack[ll_sp - 1] = LLVMConstOr(tmp[0], tmp[1]);
		break;
	case EQEQ:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntEQ, tmp[0], tmp[1], "eq");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntEQ, tmp[0], tmp[1]);
		break;
	case EQEQR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealOEQ, tmp[0], tmp[1], "eq");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealOEQ, tmp[0], tmp[1]);
		break;
	case NOTEQ:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntNE, tmp[0], tmp[1], "not_eq");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntNE, tmp[0], tmp[1]);
		break;
	case NOTEQR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealONE, tmp[0], tmp[1], "not_eq");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealONE, tmp[0], tmp[1]);
		break;
	case LLT:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntSLT, tmp[0], tmp[1], "lt");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntSLT, tmp[0], tmp[1]);
		break;
	case LLTR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealOLT, tmp[0], tmp[1], "lt");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealOLT, tmp[0], tmp[1]);
		break;
	case LGT:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntSGT, tmp[0], tmp[1], "gt");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntSGT, tmp[0], tmp[1]);
		break;
	case LGTR:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealOGT, tmp[0], tmp[1], "gt");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealOGT, tmp[0], tmp[1]);
	case LLE:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntSLE, tmp[0], tmp[1], "le");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntSLE, tmp[0], tmp[1]);
		break;
	case LLER:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealOLE, tmp[0], tmp[1], "le");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealOLE, tmp[0], tmp[1]);
		break;
	case LGE:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildICmp(ll_builder, LLVMIntSGE, tmp[0], tmp[1], "ge");
		else
			ll_stack[ll_sp - 1] = LLVMConstICmp(LLVMIntSGE, tmp[0], tmp[1]);
		break;
	case LGER:
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildFCmp(ll_builder, LLVMRealOGE, tmp[0], tmp[1], "ge");
		else
			ll_stack[ll_sp - 1] = LLVMConstFCmp(LLVMRealOGE, tmp[0], tmp[1]);
		break;
	default:
		break;
	}
	ll_stack[ll_sp - 2] = LLCOMP;
}

void ll_tocode(int c)
{
	LLVMValueRef tmp[2] = { NULL, NULL };

	if ((c >= ASS && c <= DIVASS) ||
		(c >= ASSV && c <= DIVASSV) ||
		(c >= PLUSASSR && c <= DIVASSR) ||
		(c >= PLUSASSRV && c <= DIVASSRV) ||
		(c >= POSTINC && c <= DEC) ||
		(c >= POSTINCV && c <= DECV) ||
		(c >= POSTINCR && c <= DECR) ||
		(c >= POSTINCRV && c <= DECRV))
		ll_ass_fun(c);
	else if ((c >= ASSAT && c <= DIVASSAT) ||
		(c >= ASSATV && c <= DIVASSATV) ||
		(c >= PLUSASSATR && c <= DIVASSATR) ||
		(c >= PLUSASSATRV && c <= DIVASSATRV) ||
		(c >= POSTINCAT && c <= DECAT) ||
		(c >= POSTINCATV && c <= DECATV) ||
		(c >= POSTINCATR && c <= DECATR) ||
		(c >= POSTINCATRV && c <= DECATRV))
		ll_assat(c);
	else if ((c >= LREM && c <= LDIV && c != LOGAND && c != LOGOR) ||
		(c >= EQEQR && c <= LDIVR))
		ll_binop(c);
	else
	switch (c) {
	case LOAD:
		ll_stack[ll_sp - 2] = LLCOMP;
		ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildLoad(ll_builder, ll_identab[ll_stack[ll_sp - 1]], "load");
		LLVMSetAlignment(ll_stack[ll_sp - 1], ansttype == LCHAR ? 1 : 4);
		break;
	case WIDEN:
	{
		LLVMValueRef val = NULL;
		ansttype = LINT;
		val = ll_ret_last_val();
		ll_sp += 2;
		ll_stack[ll_sp - 2] = LLCOMP;
		ll_stack[ll_sp - 1] = LLVMBuildSIToFP(ll_builder, val, ll_type_ref(LFLOAT), "widen");
	}
	break;
	case WIDEN1:
	{
		LLVMValueRef val = NULL;
		ll_sp -= 2;
		ansttype = LINT;
		val = ll_ret_last_val();
		ll_sp += 2;
		ll_stack[ll_sp - 2] = LLCOMP;
		ll_stack[ll_sp - 1] = LLVMBuildSIToFP(ll_builder, val, ll_type_ref(LFLOAT), "widen");
		ll_sp += 2;
	}
		break;
	case SLICE:
	{
		LLVMValueRef indices[] = { ll_ret_const(LINT, 0), NULL };
		indices[1] = ll_ret_last_val();
		ll_stack[ll_sp - 1] = LLVMBuildGEP(ll_builder, ll_identab[ll_stack[ll_sp - 1]], indices, 2, "pt");
		ll_stack[ll_sp - 2] = LLCOMP;
//		ll_sp -= 2;
		break;
	}
	case LAT:
		ll_stack[ll_sp + 1] = LLVMBuildLoad(ll_builder, ll_ret_last_val(), "ld");
		LLVMSetAlignment(ll_stack[ll_sp + 1], ansttype == LCHAR ? 1 : 4);
		ll_sp += 2;
		break;

	case DECX:
		ll_sp -= 2;
		break;

	case UNMINUS:
	case UNMINUSR:

	case LOGAND:
		break;

	case LOGOR:

	case ANDADDR:
	case MULTADDR:

	case LNOT:
	case LOGNOT:

	default:
		break;
	}
}

void ll_constr_print_call(int type) {
	LLVMTypeRef ll_type = NULL;
	LLVMTypeRef params[] = { NULL };
	LLVMValueRef args[] = { NULL, NULL };
	LLVMValueRef str = NULL;
	LLVMBasicBlockRef entry;
	LLVMValueRef indices[] = { ll_ret_const(LINT, 0) };
	switch (type)
	{
	case LINT:
		params[0] = ll_type_ref(LINT);
		type = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
		print[-LINT] = ll_func = LLVMAddFunction(ll_module, "printi32", type);
		strcpy(ll_reprtab, "%d\0");
		entry = LLVMAppendBasicBlock(ll_func, "entry");
		LLVMPositionBuilderAtEnd(ll_builder, entry);
		str = LLVMBuildGlobalString(ll_builder, ll_reprtab, "printi32.str");
		break;
	case LFLOAT:
		params[0] = LLVMDoubleType();
		type = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
		print[-LFLOAT] = ll_func = LLVMAddFunction(ll_module, "printfloat", type);
		strcpy(ll_reprtab, "%f\0");
		entry = LLVMAppendBasicBlock(ll_func, "entry");
		LLVMPositionBuilderAtEnd(ll_builder, entry);
		str = LLVMBuildGlobalString(ll_builder, ll_reprtab, "printfloat.str");
		break;
	case LCHAR:
	case ROWOFINT:
	default:
		return NULL;
		break;
	}
	args[1] = LLVMGetParam(ll_func, 0);
	args[0] = LLVMBuildGEP(ll_builder, str, indices, 1, "f");
	args[0] = LLVMBuildPointerCast(ll_builder, args[0], LLVMPointerType(LLVMInt8Type(), 0), "str");
	LLVMBuildCall(ll_builder, ll_printf, args, 2, "f");
	LLVMBuildRetVoid(ll_builder);
}

void ll_create_print() {
	int i = -LINT;
	for (i; i < -ROWROWOFFLOAT; i++)
		ll_constr_print_call(-i);
}

LLVMValueRef ll_constr_printid_call(int type) {
	/*
	LLVMTypeRef ll_type = NULL;
	LLVMTypeRef params[] = { NULL };
	LLVMValueRef args[] = { NULL, NULL };
	LLVMValueRef str = NULL;
	LLVMBasicBlockRef entry;
	LLVMValueRef indices[] = { ll_ret_const(LINT, 0) };
	switch (type) {
	case LINT:
		params[0] = ll_type_ref(LINT);
		type = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
		print[-LINT] = ll_func = LLVMAddFunction(ll_module, "printi32", type);
		strcpy(ll_reprtab, "%d\0");
		entry = LLVMAppendBasicBlock(ll_func, "entry");
		LLVMPositionBuilderAtEnd(ll_builder, entry);
		str = LLVMBuildGlobalString(ll_builder, ll_reprtab, "printi32.str");
		args[1] = LLVMGetParam(ll_func, 0);
		args[0] = LLVMBuildGEP(ll_builder, str, indices, 1, "f");
		args[0] = LLVMBuildPointerCast(ll_builder, args[0], LLVMPointerType(LLVMInt8Type(), 0), "str");
		LLVMBuildCall(ll_builder, ll_printf, args, 2, "f");
		LLVMBuildRetVoid(ll_builder);
		
		break;
	}
	*/
	return NULL;
}

int ll_extract_value(int tc) {
	int l = 0, r = 0;
	do {
		switch (tree[tc])
		{
		case TInit:
			return tree[tc + 1];
		case TConst:
			l = r;
			r = tree[++tc];
			break;
		case LPLUS:
			r += l;
			l = 0;
			break;
		case LMINUS:
			r = l - r;
			l = 0;
			break;
		default:
			break;
		}
	} while (tree[++tc] != TExprend);
	return r;
}
#endif

void tocode(int c)
{
	//    printf("pc %i) %i\n", pc, c);
	mem[pc++] = c;
}

void adbreakend()
{
	while (adbreak)
	{
		int r = mem[adbreak];
		mem[adbreak] = pc;
		adbreak = r;
	}
}

void adcontend()
{
	while (adcont)
	{
		int r = mem[adcont];
		mem[adcont] = pc;
		adcont = r;
	}
}

void Expr_gen()
{
	int flagprim = 1, c;
	while (flagprim)
	{
		switch (tree[tc++])
		{
		case TIdent:
		{
			lastid = tree[tc++];
			anstdispl = identab[lastid + 3];
			ansttype = identab[lastid + 2];
#ifdef LLGEN
			ll_stack[ll_sp++] = TIdent;
			ll_stack[ll_sp++] = lastid;
#endif
		}
		break;

		case TIdenttoval:
		{
			lastid = tree[tc++];
			anstdispl = identab[lastid + 3];
			tocode(LOAD);
			tocode(anstdispl);
#ifdef LLGEN
			ll_tocode(LOAD);
#endif
		}
		break;

		case TAddrtoval:
		{
			tocode(LAT);
#ifdef LLGEN
			ll_tocode(LAT);
#endif
		}
		break;

		case TConst:
		{
			tocode(LI);
			tocode(tree[tc++]);
#ifdef LLGEN
			ll_stack[ll_sp++] = TConst;
			ll_stack[ll_sp++] = tree[tc - 1];
#endif
		}
		break;

		case TString:
		{
			int n = -1, res;
			tocode(LI);
			tocode(res = pc + 4);
			tocode(RUCB);
			pc += 2;
			do
				n++, tocode(tree[tc]);
			while (tree[tc++]);

			mem[res - 1] = n;
			mem[res - 2] = pc;
		}
		break;

		case TSliceident:
		{
			int displ = identab[tree[tc++] + 3];
			tocode(LOAD);
			tocode(displ);
			Expr_gen();
			tocode(SLICE);
#ifdef LLGEN
			ll_tocode(SLICE);
			ansttype += 4;
#endif
		}
		break;

		case TSlice:
		{
			tocode(LAT);
			//                Expr_gen();
			tocode(SLICE);
		}
		break;

		case TCall1:
		{
			int i, n = tree[tc++];
			tocode(CALL1);
			for (i = 0; i < n; i++){
				Expr_gen();
			}
		}
		break;

		case TCall2:
		{
#ifdef LLGEN
			ll_stack[ll_sp++] = TCall2;
			ll_stack[ll_sp++] = LLVMBuildCall(ll_builder, ll_identab[-tree[tc]], NULL, 0, /*LLVMGetValueName(ll_identab[-tree[tc]])*/ "");
			ll_tocode(CALL2);
#endif
			tocode(CALL2);
			tocode(identab[-tree[tc++] + 3]);
		}
		break;
		}
		while ((c = tree[tc]) > 0)
		{
			tc++;
			tocode(c);
#ifdef LLGEN
			ll_tocode(c);
#endif
			if ((c >= ASS && c <= DIVASS) || (c >= ASSV && c <= DIVASSV) ||
				(c >= PLUSASSR && c <= DIVASSR) || (c >= PLUSASSRV && c <= DIVASSRV) ||
				(c >= POSTINC && c <= DEC) || (c >= POSTINCV && c <= DECV) ||
				(c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
			{
				tocode(identab[-tree[tc++] + 3]);
			}
		}

		if (tree[tc] == TExprend)
		{
			tc++;
			flagprim = 0;
		}
	}
}

/*
    else if (cur == IDENT && next == COLON)
    {
        int ppc, rr1;
        flagsemicol = 0;
        if ( (rr1 = reprtab[repr+1]) > 0 && (ppc = identab[rr1+3]) < 0)
        {
            ppc = -ppc;
            identab[rr1+3] = pc;
            while (ppc)
            {
                int r = mem[ppc];
                mem[ppc] = pc;
                ppc = r;
            }
        }
        else
            toidentab(1);
        scaner();
        statement();
    }
            case LGOTO:
            {
                int ppc, rr1;
                mustbe(IDENT, no_ident_after_goto);
                tocode(B);
                
                if ( (rr1 = reprtab[repr+1]) > 0)
                {
                    if ( (ppc = identab[rr1+3]) > 0)
                        tocode(ppc);
                    else
                    {
                        mem[pc] = -ppc;
                        identab[rr1+3] = -pc;
                        pc++;
                    }
                }
                else
                {
                    int labid = toidentab(1);
                    identab[labid+3] = -pc;
                    mem[pc++] = 0;
                }
            }
*/
void compstmt_gen();

void Stmt_gen()
{
	switch (tree[tc++])
	{
	case TBegin:
	{
#ifdef LLGEN
//		LLVMAppendBasicBlock(ll_func, ll_reprtab);
#endif
		compstmt_gen();
	}
	break;

	case TIf:
	{
		int thenref = tree[tc++], elseref = tree[tc++], ad;
#ifdef LLGEN
		LLVMBasicBlockRef then_b, else_b, end_b, prevend = ll_funend;
		ll_funend = end_b = LLVMInsertBasicBlock(ll_funend, "if.end");
		LLVMWriteBitcodeToFile(ll_module, "test.bc");
		if (elseref)
			else_b = LLVMInsertBasicBlock(end_b, "if.else");
		else
			else_b = end_b;
		then_b = LLVMInsertBasicBlock(else_b, "if.then");
#endif
		_box = VAL;
		Expr_gen();
		tocode(BE0);
		ad = pc++;
		_box = DECX;
#ifdef LLGEN
		LLVMBuildCondBr(ll_builder, ll_ret_last_val(), then_b, else_b);
		LLVMPositionBuilderAtEnd(ll_builder, then_b);
#endif
		Stmt_gen();
#ifdef LLGEN
		LLVMBuildBr(ll_builder, end_b);
		LLVMPositionBuilderAtEnd(ll_builder, else_b);
#endif
		if (elseref)
		{
			mem[ad] = pc + 2;
			tocode(RUCB);
			ad = pc++;
			Stmt_gen();
#ifdef LLGEN
			LLVMBuildBr(ll_builder, end_b);
			LLVMPositionBuilderAtEnd(ll_builder, end_b);
#endif
		}
		mem[ad] = pc;
#ifdef LLGEN
		ll_funend = prevend;
#endif
	}
	break;

	case TWhile:
	{
		int doref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc;
#ifdef LLGEN
		LLVMBasicBlockRef cond, body, end, prevend = ll_funend;
		ll_funend = end = LLVMInsertBasicBlock(ll_funend, "while.end");
		body = LLVMInsertBasicBlock(end, "while.body");
		ll_continue = cond = LLVMInsertBasicBlock(body, "while.cond");

		LLVMBuildBr(ll_builder, cond);
		LLVMPositionBuilderAtEnd(ll_builder, cond);
#endif
		adcont = 0;
		_box = VAL;
		Expr_gen();
#ifdef LLGEN
		LLVMBuildCondBr(ll_builder, ll_ret_last_val(), body, end);
		LLVMPositionBuilderAtEnd(ll_builder, body);
#endif
		tocode(BE0);
		mem[pc] = 0;
		adbreak = pc++;
		_box = DECX;
		Stmt_gen();
		adcontend();
		tocode(RUCB);
		tocode(ad);
		adbreakend();
		adbreak = oldbreak;
		adcont = oldcont;
#ifdef LLGEN
		LLVMBuildBr(ll_builder, cond);
		LLVMPositionBuilderAtEnd(ll_builder, end);
		ll_funend = prevend;
#endif
	}
	break;

	case TDo:
	{
		int condref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc;
#ifdef LLGEN
		LLVMBasicBlockRef cond, body, end, prevend = ll_funend;
		ll_funend = end = LLVMInsertBasicBlock(ll_funend, "do.end");
		cond = LLVMInsertBasicBlock(end, "do.cond");
		ll_continue = body = LLVMInsertBasicBlock(cond, "do.body");

		LLVMBuildBr(ll_builder, body);
		LLVMPositionBuilderAtEnd(ll_builder, body);
#endif
		adbreak = 0;
		adcont = 0;
		_box = DECX;
		Stmt_gen();
		adcontend();
		_box = VAL;
#ifdef LLGEN
		LLVMBuildBr(ll_builder, cond);
		LLVMPositionBuilderAtEnd(ll_builder, cond);
#endif
		Expr_gen();
		tocode(BNE0);
		tocode(ad);
		adbreakend();
		adbreak = oldbreak;
		adcont = oldcont;
#ifdef LLGEN
		LLVMBuildCondBr(ll_builder, ll_ret_last_val(), body, end);
		LLVMPositionBuilderAtEnd(ll_builder, end);
		ll_funend = prevend;
#endif
	}
	break;

	case TFor:
	{
		int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++], stmtref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc, incrtc, endtc;
#ifdef LLGEN
		LLVMBasicBlockRef cond, body, inc, end, prevend = ll_funend;
		ll_funend = end = LLVMInsertBasicBlock(ll_funend, "for.end");
		ll_continue = inc = LLVMInsertBasicBlock(end, "for.inc");
		body = LLVMInsertBasicBlock(inc, "for.body");
		cond = LLVMInsertBasicBlock(body, "for.cond");
#endif
		adcont = 0;
		if (fromref)
		{
			Expr_gen();         // init
		}
#ifdef LLGEN
		LLVMBuildBr(ll_builder, cond);
		LLVMPositionBuilderAtEnd(ll_builder, cond);
#endif
		ad = pc;
		if (condref)
		{
			Expr_gen();         // cond
			tocode(BE0);
			mem[pc] = 0;
			adbreak = pc++;
		}
#ifdef LLGEN
		LLVMBuildCondBr(ll_builder, ll_ret_last_val(), body, end);
		LLVMPositionBuilderAtEnd(ll_builder, body);
#endif
		incrtc = tc;
		tc = stmtref;
		_box = DECX;
		Stmt_gen();
		adcontend();
#ifdef LLGEN
		LLVMBuildBr(ll_builder, inc);
		LLVMPositionBuilderAtEnd(ll_builder, inc);
#endif
		if (incrref)
		{
			endtc = tc;
			tc = incrtc;
			Expr_gen();         // incr
			tc = endtc;
		}
#ifdef LLGEN
		LLVMBuildBr(ll_builder, cond);
		LLVMPositionBuilderAtEnd(ll_builder, end);
#endif
		tocode(RUCB);
		tocode(ad);
		adbreakend();
		adbreak = oldbreak;
		adcont = oldcont;
#ifdef LLGEN
		ll_funend = prevend;
#endif

	}
	break;

	case TGoto:

	case TLabel:

	case TSwitch:
	{
		int oldbreak = adbreak, oldcase = adcase;
		adbreak = 0;
		adcase = 0;
		_box = VAL;
		Expr_gen();
		_box = DECX;
		Stmt_gen();
		if (adcase > 0)
			mem[adcase] = pc;
		adcase = oldcase;
		adbreakend();
		adbreak = oldbreak;
	}
	break;

	case TCase:
	{
		if (adcase)
			mem[adcase] = pc;
		tocode(DOUBLE);
		_box = VAL;
		Expr_gen();
		tocode(EQEQ);
		tocode(BE0);
		adcase = pc++;
		_box = DECX;
		Stmt_gen();
	}
	break;

	case TDefault:
	{
		if (adcase)
			mem[adcase] = pc;
		_box = DECX;
		Stmt_gen();
	}
	break;

	case TBreak:
	{
		tocode(RUCB);
		mem[pc] = adbreak;
		adbreak = pc++;
#ifdef LLGEN
		LLVMBuildBr(ll_builder, ll_funend);
#endif
	}
	break;

	case TContinue:
	{
		tocode(RUCB);
		mem[pc] = adcont;
		adcont = pc++;
#ifdef LLGEN
		LLVMBuildBr(ll_builder, ll_continue);
#endif
	}
	break;

	case TReturn:
	{
		tocode(RETURNV);
#ifdef LLGEN
		LLVMBuildRetVoid(ll_builder);
#endif
	}
	break;

	case TReturnval:
	{
		_box = VAL;
		Expr_gen();
		tocode(_RETURN);
#ifdef LLGEN
		ansttype = modetab[moderef] + 8;
		LLVMBuildRet(ll_builder, ll_ret_last_val());
#endif
	}
	break;

	case TPrint:
	{
#ifdef LLGEN
		LLVMValueRef param[] = {ll_ret_last_val()};
		LLVMBuildCall(ll_builder, print[-ansttype], param, 1, "");
//		LLVMBuildCall(ll_builder, ll_printf, )
#endif
		tocode(PRINT);
		tocode(tree[tc++]);  // type
	}
	break;

	case TPrintid:
	{
#ifdef LLGEN
		LLVMValueRef param[] = { ll_ret_last_val() };
		LLVMValueRef str, args[100];
		LLVMValueRef indices[] = { ll_ret_const(LINT, 0) };
		int len = ll_repr_cpy(identab[tree[tc] + 1]), i, argnum = 1;

		--len;
		switch (identab[tree[tc] + 2]) {
		case LINT:
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '=';
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '%';
			ll_reprtab[++len] = 'd';
			ll_reprtab[++len] = '\n';
			ll_reprtab[++len] = '\0';
			args[1] = LLVMBuildLoad(ll_builder, ll_identab[tree[tc]], "toprint");
			LLVMSetAlignment(args[1], ansttype == LCHAR ? 1 : 4);
			break;
		case LFLOAT:
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '=';
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '%';
			ll_reprtab[++len] = 'f';
			ll_reprtab[++len] = '\n';
			ll_reprtab[++len] = '\0';
			args[1] = LLVMBuildLoad(ll_builder, ll_identab[tree[tc]], "toprint");
			LLVMSetAlignment(args[1], ansttype == LCHAR ? 1 : 4);
			args[1] = LLVMBuildFPExt(ll_builder, args[1], LLVMDoubleType(), "conv");
			break;
		case LCHAR:
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '=';
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '%';
			ll_reprtab[++len] = 'c';
			ll_reprtab[++len] = '\n';
			ll_reprtab[++len] = '\0';
			args[1] = LLVMBuildLoad(ll_builder, ll_identab[tree[tc]], "toprint");
			LLVMSetAlignment(args[1], ansttype == LCHAR ? 1 : 4);
			break;
		case ROWOFINT:
		case ROWOFCHAR:
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '=';
			argnum = ll_identab[tree[tc] + 3];
			for (i = 0; i < argnum; i++) {
				LLVMValueRef indices[] = { ll_ret_const(LINT, i) };
				ll_reprtab[++len] = ' ';
				ll_reprtab[++len] = '%';
				ll_reprtab[++len] = 'd';
				args[1 + i] = LLVMBuildGEP(ll_builder, ll_identab[tree[tc]], indices, 1, "point");
				args[1 + i] = LLVMBuildLoad(ll_builder, args[1 + i], "ld");
				LLVMSetAlignment(args[1 + i], ansttype == LCHAR ? 1 : 4);
			}
			ll_reprtab[++len] = '\n';
			ll_reprtab[++len] = '\0';
			break;
		case ROWOFFLOAT:
			ll_reprtab[++len] = ' ';
			ll_reprtab[++len] = '=';
			argnum = ll_identab[tree[tc] + 3];
			for (i = 0; i < argnum; i++) {
				LLVMValueRef indices[] = { ll_ret_const(LINT, 0), ll_ret_const(LINT, i)};
				ll_reprtab[++len] = ' ';
				ll_reprtab[++len] = '%';
				ll_reprtab[++len] = 'f';
				args[1 + i] = LLVMBuildGEP(ll_builder, ll_identab[tree[tc]], indices, 2, "point");
				args[1 + i] = LLVMBuildLoad(ll_builder, args[1 + i], "ld");
				LLVMSetAlignment(args[1 + i], ansttype == LCHAR ? 1 : 4);
				args[1 + i] = LLVMBuildFPExt(ll_builder, args[1 + i], LLVMDoubleType(), "conv");
			}
			ll_reprtab[++len] = '\n';
			ll_reprtab[++len] = '\0';
			break;
		}
		str = LLVMBuildGlobalString(ll_builder, ll_reprtab, "printidstr");
		args[0] = LLVMBuildGEP(ll_builder, str, indices, 1, "f");
		args[0] = LLVMBuildPointerCast(ll_builder, args[0], LLVMPointerType(LLVMInt8Type(), 0), "str");
		LLVMBuildCall(ll_builder, ll_printf, args, argnum + 1, "f");
#endif
		tocode(PRINTID);
		tocode(tree[tc++]);  // ссылка в identtab
	}
	break;

	case TGetid:
	{
		tocode(GETID);
		tocode(tree[tc++]);  // ссылка в identtab
	}
	break;

	case TGetansensor:
	{
		tocode(GETANSENSOR);
	}
	break;

	case TGetdigsensor:
	{
		tocode(GETDIGSENSOR);
	}
	break;

	case TSetmotor:
	{
		tocode(SETMOTOR);
	}
	break;

	case TSleep:
	{
		tocode(SLEEP);
	}
	break;

	default:
	{
		tc--;
		Expr_gen();
	}
	break;
	}
}

void Declid_gen()
{
	int identref = tree[tc++], initref = tree[tc++], N = tree[tc++];
	int olddispl = identab[identref + 3], i;
	ansttype = identab[identref + 2];
	if (N == 0)
	{
#ifdef LLGEN
		ll_repr_cpy(identab[identref + 1]);
		if (level) {
			ll_identab[identref] = LLVMBuildAlloca(ll_builder, ll_type_ref(ansttype), ll_reprtab);
		}
		else {
			ll_identab[identref] = LLVMAddGlobal(ll_module, ll_type_ref(ansttype), ll_reprtab);
		}
		LLVMSetAlignment(ll_identab[identref], ansttype == LCHAR ? 1 : 4);
#endif
		if (initref)
		{
#ifdef LLGEN
			ll_stack[ll_sp++] = TIdent;
			ll_stack[ll_sp++] = identref;

			ll_identab[identref + 1] = tc;
#endif
			Expr_gen();
			tocode(ASSV);
#ifdef LLGEN
			ll_tocode(ASSV);
#endif
			tocode(olddispl);
		}
		else {
#ifdef LLGEN
			ll_stack[ll_sp++] = TIdent;
			ll_stack[ll_sp++] = identref;

			ll_stack[ll_sp++] = TConst;
			ll_stack[ll_sp++] = 0;
			ll_tocode(ASSV);
#endif
		}
	}
	else if (N == 1)
	{
#ifdef LLGEN
		LLVMValueRef arr = NULL, src = NULL;
		int arr_size = tc;
#endif
		ansttype += 4;

		Expr_gen();
#ifdef LLGEN
		ll_sp -= 2;
		ll_identab[identref + 3] = arr_size = ll_extract_value(arr_size);
		if (level) {
			ll_repr_cpy(identab[identref + 1]);
			ll_identab[identref] = arr = LLVMBuildArrayAlloca(ll_builder,
				LLVMArrayType(ll_type_ref(ansttype), arr_size),
				NULL,
				ll_reprtab);
		} else {
			ll_repr_cpy(identab[identref + 1]);
			ll_identab[identref] = arr = LLVMAddGlobal(ll_module, LLVMArrayType(ll_type_ref(ansttype), arr_size), ll_reprtab);
		}
		LLVMSetAlignment(arr, ansttype == LCHAR ? 1 : 4);
#endif
		tocode(DEFARR);
		tocode(olddispl);
		if (initref)
		{
			int L = tree[tc++ + 1];
#ifdef LLGEN
			LLVMValueRef *vals = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * L);
			LLVMValueRef indices[] = { ll_ret_const(LINT, 0) };
#endif
			tc++;               // INIT
			for (i = 0; i < L; i++) {
				Expr_gen();
#ifdef LLGEN
				vals[i] = ll_ret_last_val();
#endif
			}
#ifdef LLGEN
			src = LLVMConstArray(ll_type_ref(ansttype), vals, L);
			if (level) {
				LLVMValueRef arrg = LLVMAddGlobal(ll_module, LLVMArrayType(ll_type_ref(ansttype), L), ll_reprtab);
				LLVMValueRef args[] = { LLVMBuildBitCast(ll_builder, arr, LLVMPointerType(LLVMInt8Type(), 0), "dst"),
					LLVMConstBitCast(arrg, LLVMPointerType(LLVMInt8Type(), 0)),
					ll_ret_const(LINT, L * 4),
					ll_ret_const(LINT, 4),
					ll_ret_const(LLBOOL, 0)
				};
				LLVMSetInitializer(arrg, src);
//				LLVMAddAttribute(arrg, LLVMReadOnlyAttribute);
				LLVMBuildCall(ll_builder, ll_memcpy, args, 5, "");
			} else {
				LLVMSetInitializer(arr, src);
			}
#endif
			tocode(ASSARR);
			tocode(olddispl);
			tocode(L);
//			free(vals);
		} else {
#ifdef LLGEN
			LLVMValueRef *vals = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * arr_size);
			for (i = 0; i < arr_size; i++) {
				ll_stack[ll_sp++] = TConst;
				ll_stack[ll_sp++] = 0;
				vals[i] = ll_ret_last_val();
			}
			src = LLVMConstArray(ll_type_ref(ansttype), vals, arr_size);
			LLVMSetInitializer(arr, src);
#endif
		}
	}
	else               // N==2
	{
		Expr_gen();
		Expr_gen();
		tocode(DEFARR2);
		tocode(olddispl);
		if (initref)
		{
			tc++;               // INIT
			int L = tree[tc++];
			for (i = 0; i < L; i++)
				Expr_gen();
			tocode(ASSARR2);
			tocode(olddispl);
			tocode(L);
		}
	}
}

void compstmt_gen()
{
	while (tree[tc] != TEnd)
	{
		switch (tree[tc])
		{
		case TDeclid:
			tc++;
			Declid_gen();
			break;

		default:
			Stmt_gen();
		}
	}
	tc++;
}

void codegen()
{
	int oldtc = tc;
#ifdef LLGEN
	LLVMTypeRef vprintf[] = { LLVMPointerType(LLVMInt8Type(), 0) };
	LLVMTypeRef vmemcpy[] = { LLVMPointerType(LLVMInt8Type(), 0), LLVMPointerType(LLVMInt8Type(), 0), 
		ll_type_ref(LINT), ll_type_ref(LINT), LLVMInt1Type() };
	ll_module = LLVMModuleCreateWithName("out");
	ll_builder = LLVMCreateBuilder();
	ll_printf =	LLVMAddFunction(ll_module,
		"printf",
		LLVMFunctionType(ll_type_ref(LINT), vprintf, 1, 1));
	ll_memcpy = LLVMAddFunction(ll_module,
		"llvm.memcpy.p0i8.p0i8.i32",
		LLVMFunctionType(ll_type_ref(LVOID), vmemcpy, 5, 0));
	ll_create_print();
#endif
	tc = 0;
	while (tc < oldtc)
		switch (tree[tc++])
	{
		case TFuncdef:
		{
			int identref = tree[tc++], maxdispl = tree[tc++];
			int fn = identab[identref + 3], pred;
			int i, n;
#ifdef LLGEN
			LLVMTypeRef param_types[10] = { NULL };
			LLVMTypeRef ret_type;
			LLVMValueRef temp;
			LLVMBasicBlockRef entry, end;
#endif
			moderef = identab[identref + 2];
			functions[fn] = pc;
			tocode(FUNCBEG);
			tocode(maxdispl + 1);
			pred = pc++;
			tc++;             // TBegin
#ifdef LLGEN
			ret_type = LLVMFunctionType(ll_type_ref(modetab[moderef] + 8), param_types, 0, 0);
			ll_repr_cpy(identab[identref + 1] + 2 < 28 ? 8 : identab[identref + 1]);
			ll_func = LLVMAddFunction(ll_module, ll_reprtab, ret_type);
			ll_identab[identref] = ll_func;
			entry = LLVMAppendBasicBlock(ll_func, "entry");
			end = ll_funend = LLVMAppendBasicBlock(ll_func, "end");
			LLVMPositionBuilderAtEnd(ll_builder, entry);

			level = 1;
#endif
			compstmt_gen();
#ifdef LLGEN
			LLVMPositionBuilderAtEnd(ll_builder, end);
			LLVMBuildBr(ll_builder, end);
#endif
			mem[pred] = pc;
		}
		break;

		case TDeclid:
#ifdef LLGEN
			level = 0;
#endif
			Declid_gen();
			break;

		default:
		{
			printf("что-то не то\n");
			printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", tc, tree[tc - 2], tree[tc - 1]);
		}
	}

	if (wasmain == 0)
		error(no_main_in_program);
	tocode(CALL1);
	tocode(CALL2);
	tocode(identab[wasmain + 3]);
	tocode(STOP);
#ifdef LLGEN
	LLVMWriteBitcodeToFile(ll_module, "out.bc");
	LLVMDisposeModule(ll_module);
#endif
}
