//
//  codegen.c
//
//  Created by Andrey Terekhov on 3/10/15.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"

extern void error(int err);

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

int ll_sp = 0, ll_spl = 0;
int regs[10000] = { 0 };
LLVMValueRef ll_vals[10000] = { NULL };
int reg_param[100] = { 0 }, ll_stack[100] = { 0 }, ll_labels[100] = { 0 };
LLVMValueRef ll_val_stack[100] = { NULL };
int values_arr[100];
int strings_arr[100];
int level;
char ll_reprtab[100] = { 0 };

void ll_repr_cpy(int repr) {
	int i = 0;
	repr += 2;
	while (ll_reprtab[i] = (char)(reprtab[repr + i]))
		i++;
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

LLVMValueRef ll_extract_value() {
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

void ll_add_to_stack(int T, int val) {

}

void ll_fill_tmp(LLVMValueRef *tmp) {
	int tmp_ind = 0;
	do
	switch (ll_stack[ll_sp - 2])
	{
		case TConst:
			tmp[tmp_ind] = ll_ret_const(ansttype, ll_stack[ll_sp - 1]);
			break;
		case TIdent:
			tmp[tmp_ind] = LLVMBuildLoad(ll_builder, ll_vals[ll_stack[ll_sp - 1]], "val");
			break;
		case TIdenttoval:
			tmp[tmp_ind] = (LLVMValueRef)(void*)ll_stack[ll_sp - 1];
		case LLCOMP:
			tmp[tmp_ind] = ll_stack[ll_sp - 1];
			break;
		default:
			break;
	}
	while (ll_sp -= 2, ++tmp_ind < 2);
	ll_stack[ll_sp++] = LLCOMP;
	ll_sp++;
}

LLVMValueRef ll_ret_last_val() {
	LLVMValueRef toRet = NULL;
	if (ll_sp < 2)
		return NULL;
	switch (ll_stack[ll_sp - 2])
	{
	case TIdent:
		toRet = LLVMBuildLoad(ll_builder,
			ll_vals[ll_stack[ll_sp - 2]],
			"load");
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

void ll_tocode(int c)
{
	LLVMValueRef tmp[2] = { NULL, NULL };
	switch (c) {
	case INC:
	case ASS:
	case ASSV:
		switch (ll_stack[ll_sp - 2]) {
		case TConst:
			if (level) {
				LLVMValueRef constant = ll_ret_const(ansttype, ll_stack[ll_sp - 1]);
				ll_stack[ll_sp - 3] = (int)(void*)LLVMBuildStore(ll_builder, constant, ll_vals[ll_stack[ll_sp - 3]]);
			}
			else {
				LLVMSetInitializer(ll_vals[ll_stack[ll_sp - 3]], ll_ret_const(LINT, ll_stack[ll_sp - 1]));
			}
			if (ll_sp < 4)
				break;
			break;
		case TCall2:
			break;
		case TAddrtoval:
		case LLCOMP:
			ll_stack[ll_sp - 3] = (int)(void*)LLVMBuildStore(ll_builder, ll_stack[ll_sp - 1], ll_vals[ll_stack[ll_sp - 3]]);
			break;
		}
		ll_stack[ll_sp - 4] = LLCOMP;
		ll_sp -= 4;
		if (c == ASS)
			ll_sp += 2;
		break;
	case LPLUS:
		ll_fill_tmp(tmp);
		if (level)
			ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildAdd(ll_builder, tmp[0], tmp[1], "add");
		else
			ll_stack[ll_sp - 1] = (int)(void*)LLVMConstAdd(tmp[0], tmp[1]);
		break;
	case LREM:
		ll_fill_tmp(tmp);
		ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildSRem(ll_builder, tmp[0], tmp[1], "srem");
		break;
	case LOAD:
		ll_stack[ll_sp - 2] = TIdenttoval;
		ll_stack[ll_sp - 1] = (int)(void*)LLVMBuildLoad(ll_builder, ll_vals[ll_stack[ll_sp - 1]], "load");
		break;
	default:
		break;
	}
/*	if ((c >= ASS && c <= DIVASS) || (c >= ASSV && c <= DIVASSV) ||
		(c >= PLUSASSR && c <= DIVASSR) || (c >= PLUSASSRV && c <= DIVASSRV) ||
		(c >= POSTINC && c <= DEC) || (c >= POSTINCV && c <= DECV) ||
		(c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))*/
}

LLVMValueRef ll_constr_print_call(int type) {
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
		strcpy(ll_reprtab, "%d\n\0");
		entry = LLVMAppendBasicBlock(ll_func, "entry");
		LLVMPositionBuilderAtEnd(ll_builder, entry);
		str = LLVMBuildGlobalString(ll_builder, ll_reprtab, "printi32.str");
		args[1] = LLVMGetParam(ll_func, 0);
		args[0] = LLVMBuildGEP(ll_builder, str, indices, 1, "f");
		args[0] = LLVMBuildPointerCast(ll_builder, args[0], LLVMPointerType(LLVMInt8Type(), 0), "str");
		LLVMBuildCall(ll_builder, ll_printf, args, 2, "f");
		LLVMBuildRetVoid(ll_builder);
		break;
	case LFLOAT:
	case LCHAR:
	case ROWOFINT:
	default:
		return NULL;
		break;
	}
}

void ll_create_print() {
	int i = -LINT;
	for (i; i < -ROWROWOFFLOAT; i++)
		ll_constr_print_call(-i);
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
			ll_stack[ll_sp++] = LLVMBuildCall(ll_builder, ll_vals[-tree[tc]], NULL, 0, LLVMGetValueName(ll_vals[-tree[tc]]));
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
		compstmt_gen();
	}
	break;

	case TIf:
	{
		int thenref = tree[tc++], elseref = tree[tc++], ad;
		_box = VAL;
		Expr_gen();
		tocode(BE0);
		ad = pc++;
		_box = DECX;
		Stmt_gen();
		if (elseref)
		{
			mem[ad] = pc + 2;
			tocode(RUCB);
			ad = pc++;
			Stmt_gen();
		}
		mem[ad] = pc;
	}
	break;

	case TWhile:
	{
		int doref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc;
		adcont = 0;
		_box = VAL;
		Expr_gen();
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
	}
	break;

	case TDo:
	{
		int condref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc;
		adbreak = 0;
		adcont = 0;
		_box = DECX;
		Stmt_gen();
		adcontend();
		_box = VAL;
		Expr_gen();
		tocode(BNE0);
		tocode(ad);
		adbreakend();
		adbreak = oldbreak;
		adcont = oldcont;
	}
	break;

	case TFor:
	{
		int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++], stmtref = tree[tc++];
		int oldbreak = adbreak, oldcont = adcont, ad = pc, incrtc, endtc;
		adcont = 0;
		if (fromref)
		{
			Expr_gen();         // init
		}
		ad = pc;
		if (condref)
		{
			Expr_gen();         // cond
			tocode(BE0);
			mem[pc] = 0;
			adbreak = pc++;
		}
		incrtc = tc;
		tc = stmtref;
		_box = DECX;
		Stmt_gen();
		adcontend();
		if (incrref)
		{
			endtc = tc;
			tc = incrtc;
			Expr_gen();         // incr
			tc = endtc;
		}
		tocode(RUCB);
		tocode(ad);
		adbreakend();
		adbreak = oldbreak;
		adcont = oldcont;
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
	}
	break;

	case TContinue:
	{
		tocode(RUCB);
		mem[pc] = adcont;
		adcont = pc++;
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
		LLVMBuildRet(ll_builder, ll_ret_last_val());
#endif
	}
	break;

	case TPrint:
	{
#ifdef LLGEN
		LLVMValueRef param[] = {ll_ret_last_val()};
		LLVMBuildCall(ll_builder, print[-ansttype], param, 1, "\0");
#endif
		tocode(PRINT);
		tocode(tree[tc++]);  // type
	}
	break;

	case TPrintid:
	{
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
			ll_vals[identref] = LLVMBuildAlloca(ll_builder, ll_type_ref(identab[identref + 2]), ll_reprtab);
		}
		else {
			ll_vals[identref] = LLVMAddGlobal(ll_module, ll_type_ref(identab[identref + 2]), ll_reprtab);
		}
#endif
		if (initref)
		{
#ifdef LLGEN
			ll_stack[ll_sp++] = TIdent;
			ll_stack[ll_sp++] = identref;
#endif
			Expr_gen();
			tocode(ASSV);
#ifdef LLGEN
			ll_tocode(ASSV);
#endif
			tocode(olddispl);
		}
	}
	else if (N == 1)
	{
#ifdef LLGEN
		LLVMValueRef arr = NULL, src = NULL;
#endif
		ansttype += 4;

		Expr_gen();
#ifdef LLGEN
//		TODO;
//		ll_extract_value();
		if (level) {
			ll_repr_cpy(identab[identref + 1]);
			arr = LLVMBuildArrayAlloca(ll_builder, 
				LLVMArrayType(ll_type_ref(identab[identref + 2] + 4), 3),
				NULL,
				ll_reprtab);
		}
		else {
			ll_repr_cpy(identab[identref + 1]);
			arr = LLVMAddGlobal(ll_module, LLVMArrayType(ll_type_ref(identab[identref + 2] + 4), ll_ret_last_val()), ll_reprtab);
		}
#endif
		tocode(DEFARR);
		tocode(olddispl);
		if (initref)
		{
			int L = tree[tc++ + 1];
			LLVMValueRef *vals = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * L);
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
				LLVMValueRef args[] = { LLVMBuildBitCast(ll_builder, arr, LLVMPointerType(LLVMInt8Type(), 0), "dst"),
					LLVMConstBitCast(src, LLVMPointerType(LLVMInt8Type(), 0)),
					ll_ret_const(LINT, L * 4),
					ll_ret_const(LINT, 4),
					ll_ret_const(LLBOOL, 0)
				};
				LLVMBuildCall(ll_builder, ll_memcpy, args, 5, "funcall");
			}
#endif
			tocode(ASSARR);
			tocode(olddispl);
			tocode(L);
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
			int i, n, moderef = identab[identref + 2];
#ifdef LLGEN
			LLVMTypeRef param_types[10] = { NULL };
			LLVMTypeRef ret_type;
			LLVMValueRef temp;
			LLVMBasicBlockRef entry;
#endif
			functions[fn] = pc;
			tocode(FUNCBEG);
			tocode(maxdispl + 1);
			pred = pc++;
			tc++;             // TBegin
#ifdef LLGEN
			ret_type = LLVMFunctionType(ll_type_ref(modetab[moderef] + 8), param_types, 0, 0);
			ll_repr_cpy(identab[identref + 1] + 2 < 28 ? 8 : identab[identref + 1]);
			ll_func = LLVMAddFunction(ll_module,
				ll_reprtab,
				ret_type);
			ll_vals[identref] = ll_func;
			entry = LLVMAppendBasicBlock(ll_func, "entry");
			LLVMPositionBuilderAtEnd(ll_builder, entry);

			level = 1;

			// TODO by ANT
			/*
			if (n > 0) {
			ll_type_print(modetab[moderef + 2]);
			fprintf(ll_out, " ");
			ll_repr_print(modetab[moderef + 3]);
			}
			for (i = 1; i < n; i++){
			fprintf(ll_out, ", ");
			ll_type_print(modetab[moderef + 2 + 2 * i]);
			fprintf(ll_out, " ");
			ll_repr_print(modetab[moderef + 3 + 2 * i]);
			}
			*/
#endif
			compstmt_gen();
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
