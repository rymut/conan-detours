#include <stdio.h>
#include <windows.h>

#include <detours.h>


static VOID Dump(PBYTE pbBytes, LONG nBytes, PBYTE pbTarget)
{
	for (LONG n = 0; n < nBytes; n += 16) {
		printf("    %p: ", pbBytes + n);
		for (LONG m = n; m < n + 16; m++) {
			if (m >= nBytes) {
				printf("  ");
			}
			else {
				printf("%02x", pbBytes[m]);
			}
			if (m % 4 == 3) {
				printf(" ");
			}
		}
		if (n == 0 && pbTarget != DETOUR_INSTRUCTION_TARGET_NONE) {
			printf(" [%p]", pbTarget);
		}
		printf("\n");
	}
}

static VOID Decode(PCSTR pszDesc, PBYTE pbCode, PBYTE pbOther, PBYTE pbPointer, LONG nInst)
{
	if (pbCode != pbPointer) {
		printf("  %s = %p [%p]\n", pszDesc, pbCode, pbPointer);
	}
	else {
		printf("  %s = %p\n", pszDesc, pbCode);
	}

	if (pbCode == pbOther) {
		printf("    ... unchanged ...\n");
		return;
	}

	PBYTE pbSrc = pbCode;
	PBYTE pbEnd;
	PVOID pbTarget;
	for (LONG n = 0; n < nInst; n++) {
		pbEnd = (PBYTE)DetourCopyInstruction(NULL, NULL, pbSrc, &pbTarget, NULL);
		Dump(pbSrc, (int)(pbEnd - pbSrc), (PBYTE)pbTarget);
		pbSrc = pbEnd;
	}
}


VOID WINAPI Verify(PCHAR pszFunc, PVOID pvPointer)
{
	PVOID pvCode = DetourCodeFromPointer(pvPointer, NULL);

	Decode(pszFunc, (PBYTE)pvCode, NULL, (PBYTE)pvPointer, 3);
}

VOID WINAPI VerifyEx(PCHAR pszFunc, PVOID pvPointer, LONG nInst)
{
	PVOID pvCode = DetourCodeFromPointer(pvPointer, NULL);

	Decode(pszFunc, (PBYTE)pvCode, NULL, (PBYTE)pvPointer, nInst);
}


//////////////////////////////////////////////////////////////// Target Class.
//
class CMember
{
public:
	void Target(void);
};

void CMember::Target(void)
{
	printf("  CMember::Target!   (this:%p)\n", this);
}

//////////////////////////////////////////////////////////////// Detour Class.
//
class CDetour /* add ": public CMember" to enable access to member variables... */
{
public:
	void Mine_Target(void);
	static void (CDetour::* Real_Target)(void);

	// Class shouldn't have any member variables or virtual functions.
};

void CDetour::Mine_Target(void)
{
	printf("  CDetour::Mine_Target! (this:%p)\n", this);
	(this->*Real_Target)();
}

void (CDetour::* CDetour::Real_Target)(void) = (void (CDetour::*)(void)) & CMember::Target;

//////////////////////////////////////////////////////////////////////////////
//
int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	//////////////////////////////////////////////////////////////////////////
	//

	void (CMember:: * pfTarget)(void) = &CMember::Target;
	void (CDetour:: * pfMine)(void) = &CDetour::Mine_Target;

	Verify("CMember::Target      ", *(PBYTE*)&pfTarget);
	Verify("*CDetour::Real_Target", *(PBYTE*)&CDetour::Real_Target);
	Verify("CDetour::Mine_Target ", *(PBYTE*)&pfMine);

	printf("\n");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)CDetour::Real_Target,
		*(PBYTE*)&pfMine);

	LONG l = DetourTransactionCommit();
	printf("DetourTransactionCommit = %ld\n", l);
	printf("\n");

	Verify("CMember::Target      ", *(PBYTE*)&pfTarget);
	Verify("*CDetour::Real_Target", *(&(PBYTE&)CDetour::Real_Target));
	Verify("CDetour::Mine_Target ", *(PBYTE*)&pfMine);
	printf("\n");

	//////////////////////////////////////////////////////////////////////////
	//
	CMember target;

	printf("Calling CMember (w/o Detour):\n");
	(((CDetour*)&target)->*CDetour::Real_Target)();

	printf("Calling CMember (will be detoured):\n");
	target.Target();

	return 0;
}

