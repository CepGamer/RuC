//
//  codegen.c
//
//  Created by Andrey Terekhov on 3/10/15.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"

extern void error(int err);

#ifdef LLGEN
#include <llvm-c\Core.h>
#include <llvm-c\BitWriter.h>

typedef union u
{
	int i;
	float f;
} u;

FILE *ll_out;

LLVMModuleRef ll_module = NULL;
LLVMBuilderRef ll_builder = NULL;
LLVMValueRef ll_func = NULL;

int num_reg = 0, ll_sp = 0, label_num = 1, ll_spl = 0;
LLVMValueRef ll_last = NULL;
int regs[10000] = { 0 };
LLVMValueRef ll_vals[10000] = { NULL };
int reg_param[100] = { 0 }, ll_stack[100] = { 0 }, ll_labels[100] = { 0 };
LLVMValueRef ll_val_stack[100] = { NULL };
int values_arr[100];
int strings_arr[100];
int levels[10000] = { 0 };
int level;

void ll_repr_print(int level, int repr) {
	if (repr <= 25) { 
		fprintf(ll_out, "@main");
		return;
	}
	repr += 2;
	if (level)
		fprintf(ll_out, "%c", '%%');
	else
		fprintf(ll_out, "%c", '@');
	while (reprtab[repr])
	{
		fprintf(ll_out, "%c", reprtab[repr++]);
	}
}

void ll_type_print(int type) {
	switch (type)
	{
	case LCHAR:
	case FUNCCHAR:
		fprintf(ll_out, "i8");
		break;
	case LINT:
	case FUNCINT:
		fprintf(ll_out, "i32");
		break;
	case LFLOAT:
	case FUNCFLOAT:
		fprintf(ll_out, "float");
		break;
	case LVOID:
	case FUNCVOID:
		fprintf(ll_out, "void");
		break;
	default:
		fprintf(ll_out, "i32");
		break;
	}
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

void ll_label_print(int type) {
	switch (type)
	{
	case 1:
		fprintf(ll_out, "label.true%d:\n", label_num);
		break;
	case 2:
		fprintf(ll_out, "label.false%d:\n", label_num);
		break;
	default:
		fprintf(ll_out, "label%d:\n", label_num++);
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
	case LINT:
	default:
		return LLVMConstInt(LLVMInt32Type(), value, 0);
		break;
	}
}

void ll_tocode(int c)
{
	switch (c) {
	case INC:
		fprintf(ll_out, "%%%d = load ", num_reg++);
		ll_type_print(ansttype);
		fprintf(ll_out, "* ");
		ll_repr_print(levels[ll_stack[ll_sp - 1]], identab[ll_stack[ll_sp - 1] + 1]);
		fprintf(ll_out, "\n%%%d = add i32 1, %%%d\nstore i32 %%%d, i32* ", num_reg, num_reg - 1, num_reg);
		ll_repr_print(levels[ll_stack[ll_sp - 1]], identab[ll_stack[ll_sp - 1] + 1]);
		fprintf(ll_out, "\n");
		regs[ll_stack[ll_sp - 1]] = num_reg++;
		break;
	case ASS:
		fprintf(ll_out, "%%%d = ", num_reg);
		regs[ll_stack[ll_sp - 3]] = num_reg++;
	case ASSV:

		fprintf(ll_out, "store ");
		ll_type_print(ansttype);
		switch (ll_stack[--ll_sp - 1]) {
		case TConst:
			if (level) {
				LLVMValueRef constant = ll_ret_const(ansttype, ll_stack[ll_sp]);
				LLVMBuildStore(ll_builder, constant, ll_vals[ll_stack[ll_sp - 2]]);
			}
			else {
				LLVMSetInitializer(ll_vals[ll_stack[ll_sp - 2]], LLVMConstInt(LLVMInt32Type(), ll_stack[ll_sp], 0));
			}
			switch (ansttype) {
			case LINT:
				fprintf(ll_out, " %d, ", ll_stack[ll_sp]);
				break;
			case LFLOAT:
				fprintf(ll_out, " %X, ", ll_stack[ll_sp]);
				break;
			}
			ll_sp--;
			if (ll_sp < 2)
				break;
			ll_type_print(ansttype);
			fprintf(ll_out, "* ");
			ll_repr_print(levels[ll_stack[ll_sp - 1]], identab[ll_stack[ll_sp - 1] + 1]);
			fprintf(ll_out, "\n");
			ll_sp -= 2;
			break;
		case TCall2:
			LLVMBuildStore(ll_builder, ll_last, ll_vals[ll_stack[ll_sp - 2]]);
			break;
		}
		break;
	default:
		break;
	}
/*	if ((c >= ASS && c <= DIVASS) || (c >= ASSV && c <= DIVASSV) ||
		(c >= PLUSASSR && c <= DIVASSR) || (c >= PLUSASSRV && c <= DIVASSRV) ||
		(c >= POSTINC && c <= DEC) || (c >= POSTINCV && c <= DECV) ||
		(c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))*/
}

LLVMValueRef ll_constr_print(int type) {
	switch (type)
	{
	case LINT:
	default:
		break;
	}
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
			ll_last = ll_ret_const(ansttype, tree[tc - 1]);

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
			ll_last = LLVMBuildCall(ll_builder, ll_vals[-tree[tc]], NULL, 0, LLVMGetValueName(ll_vals[-tree[tc]]));

			ll_stack[ll_sp++] = TCall2;
			ll_sp++;
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

		fprintf(ll_out, "ret void\n");
#endif
	}
	break;

	case TReturnval:
	{
		_box = VAL;
		Expr_gen();
		tocode(_RETURN);
#ifdef LLGEN
		LLVMBuildRet(ll_builder, ll_last);

		fprintf(ll_out, "ret ");
		ll_type_print(ansttype);
		switch (ll_stack[ll_sp - 2]) {
		case TConst:
			fprintf(ll_out, " %d\n", ll_stack[ll_sp - 1]);
			ll_sp -= 2;
			break;
		default:
			fprintf(ll_out, " %%%d\n", num_reg - 1);
			break;
		}
#endif
	}
	break;

	case TPrint:
	{
		tocode(PRINT);
		tocode(tree[tc++]);  // type
#ifdef LLGEN
//		LLVMBuildCall();

		fprintf(ll_out, "call void @print (");
		ll_type_print(tree[tc - 1]);
		fprintf(ll_out, " %%%d)\n", num_reg - 1);
#endif
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
	if (N == 0)
	{
#ifdef LLGEN
		if (level) {
			ll_last = LLVMBuildAlloca(ll_builder, ll_type_ref(ansttype), reprtab + (identab[identref + 1] + 2));
			ll_vals[identref] = ll_last;
		}
		else {
			ll_last = LLVMAddGlobal(ll_module, ll_type_ref(ansttype), reprtab + (identab[identref + 1] + 2));
			ll_vals[identref] = ll_last;
		}


		ll_repr_print(level, identab[identref + 1]);
		if (level) {
			fprintf(ll_out, " = alloca ");
			ll_type_print(modetab[identab[identref + 2]]);
			levels[identref] = 1;
		}
		fprintf(ll_out, "\n");
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
		Expr_gen();
#ifdef LLGEN
		if (level) {
/*			LLVMBuildArrayAlloca(ll_builder, 
				LLVMArrayType(ll_type_ref(ansttype + 4), ll_last), 
				, 
				(char*)(reprtab + (identab[identref + 1] + 2)));
	*/	}
		else {
			LLVMAddGlobal(ll_module, LLVMArrayType(ll_type_ref(ansttype + 4), ll_last), (char*)(reprtab + (identab[identref + 1] + 2)));
		}
#endif
		tocode(DEFARR);
		tocode(olddispl);
		if (initref)
		{
			tc++;               // INIT
			int L = tree[tc++];
			for (i = 0; i < L; i++)
				Expr_gen();
#ifdef LLGEN
//			LLVMSetInitializer
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
#ifdef LLGEN
			ll_sp = 0;
#endif
			Stmt_gen();
		}
	}
	tc++;
}

void codegen()
{
	int oldtc = tc;
#ifdef LLGEN
	ll_module = LLVMModuleCreateWithName("out");
	ll_builder = LLVMCreateBuilder();
	
	ll_out = fopen(LLFILE, "w");
	fprintf(ll_out, "target triple = \"i686-pc-windows-gnu\"\n");
	fprintf(ll_out, "declare i32 @printf(i8*, ...)\n\n");
	fprintf(ll_out, "@.strd = private constant [3 x i8] c\"%%d\\00\"\n\n");
	fprintf(ll_out, "define void @print(i32 %%x){\ncall i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([3 x i8]* @.strd, i32 0, i32 0), i32 %%x)\nret void\n}\n\n");
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
			ll_func = LLVMAddFunction(ll_module, 
				reprtab + (identab[identref + 1] + 2 < 28 ? 10 : identab[identref + 1] + 2),
				ret_type);
			ll_vals[identref] = ll_func;
			entry = LLVMAppendBasicBlock(ll_func, "entry");
			LLVMPositionBuilderAtEnd(ll_builder, entry);

			level = num_reg = 1;
			fprintf(ll_out, "define ");
			ll_type_print(modetab[moderef]);
			fprintf(ll_out, " ");
			ll_repr_print(0, identab[identref + 1]);
			n = modetab[moderef + 1];
			fprintf(ll_out, "(");
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
			fprintf(ll_out, ") {\n");
#endif
			compstmt_gen();
#ifdef LLGEN
			fprintf(ll_out, "}\n\n");
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
	fclose(ll_out);
#endif
}
