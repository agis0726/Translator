#include "BGI.h"
//[241129][1305014][Tily] あの日の君を振り向かせて。 DL版 (files)
void BGI7Filter(TextBuffer *buffer, HookParam *)
{
  auto text = reinterpret_cast<LPWSTR>(buffer->buff);
  CharFilter(buffer, L'\x0001');
  CharFilter(buffer, L'\x0002');
  CharFilter(buffer, L'\x0003');
  CharFilter(buffer, L'\x0004');
  CharFilter(buffer, L'\x0005');
  CharFilter(buffer, L'\x000A');
  if (text[0] == L'\u3000')
  {
    buffer->size -= 2;
    ::memmove(text, text + 1, buffer->size);
  }
  CharReplacer(buffer, L'\u3000', L' '); // IDSP

  if (cpp_wcsnstr(text, L"<", buffer->size / sizeof(wchar_t)))
  {
    StringFilterBetween(buffer, L"<", 1, L">", 1);
  }
}
void BGI7FilterA(TextBuffer *buffer, HookParam *)
{
  auto text = reinterpret_cast<LPCSTR>(buffer->buff);
  StringFilterBetween(buffer, "<", 1, ">", 1);
}
bool BGIattach_function1()
{
  /*
CHAR *__fastcall sub_1400F5BC0(LPSTR lpMultiByteStr, LPCWCH lpWideCharStr)
{
  CHAR *v3; // rdi
  UINT v5; // ebx
  int v6; // r8d
  int v7; // eax
  int cbMultiByte; // ebp

  v3 = 0i64;
  v5 = sub_1400F4740();
  if ( v6 )
  {
    if ( v6 == 1 )
      v5 = 65001;
  }
  else
  {
    v5 = 932;
  }
  v7 = WideCharToMultiByte(v5, 0, lpWideCharStr, -1, 0i64, 0, 0i64, 0i64);
  cbMultiByte = v7;
  if ( v7 >= 1 )
  {
    if ( !lpMultiByteStr )
    {
      v3 = (CHAR *)operator new(v7 + 1);
      lpMultiByteStr = v3;
    }
    WideCharToMultiByte(v5, 0, lpWideCharStr, -1, lpMultiByteStr, cbMultiByte, 0i64, 0i64);
  }
  return v3;
}
.text:00000001400F5BF6                 mov     ebx, 0FDE9h
.text:00000001400F5BFB                 jmp     short loc_1400F5C02
.text:00000001400F5BFD ; ---------------------------------------------------------------------------
.text:00000001400F5BFD
.text:00000001400F5BFD loc_1400F5BFD:                          ; CODE XREF: sub_1400F5BC0+2E↑j
.text:00000001400F5BFD                 mov     ebx, 3A4h*/
  const BYTE bytes[] = {
      0xBB, 0xE9, 0xFD, 0x00, 0x00, // cp=65001
      XX2,
      0xBB, 0xA4, 0x03, 0x00, 0x00 // cp=932
  };
  auto addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if (!addr)
    return false;
  addr = MemDbg::findEnclosingAlignedFunction(addr);
  if (!addr)
    return false;
  HookParam hp;
  hp.address = addr;
  hp.type = CODEC_UTF16 | USING_STRING;
  hp.filter_fun = BGI7Filter;
  hp.offset = GETARG2;
  return NewHook(hp, "BGI");
}

bool BGIattach_function2()
{
  /*
      if ( *(unsigned __int8 *)v1 == 239 )
      {
        v12 = *((unsigned __int8 *)v1 + 1) | 0xEF00;
        goto LABEL_16;
      }
      if ( *(unsigned __int8 *)v1 == 255 )
      {
        v12 = *((unsigned __int8 *)v1 + 1) | 0xF000;
.text:000000014007051C                 movzx   eax, byte ptr [rbx+1]
.text:0000000140070520                 or      eax, 0F000h
.text:0000000140070525                 jmp     short loc_140070530
.text:0000000140070527 ; ---------------------------------------------------------------------------
.text:0000000140070527
.text:0000000140070527 loc_140070527:                          ; CODE XREF: sub_140070430+D2↑j
.text:0000000140070527                 movzx   eax, byte ptr [rbx+1]
.text:000000014007052B                 or      eax, 0EF00h*/
  const BYTE bytes[] = {
      0x0F, 0xB6, 0x43, 0x01,
      0x0D, 0x00, 0xF0, 0x00, 0x00,
      XX2,
      0x0F, 0xB6, 0x43, 0x01,
      0x0D, 0x00, 0xEF, 0x00, 0x00};
  auto addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if (!addr)
    return false;
  addr = MemDbg::findEnclosingAlignedFunction(addr);
  if (!addr)
    return false;
  auto addrs = findxref_reverse_checkcallop(addr, processStartAddress, processStopAddress, 0xe8);
  if (1 != addrs.size())
  {
    HookParam hp;
    hp.address = addrs[0];
    hp.type = USING_STRING;
    hp.filter_fun = BGI7FilterA;
    hp.offset = GETARG1;
    return NewHook(hp, "BGI");
  }
  HookParam hp;
  hp.address = addrs[0] + 5;
  hp.type = CODEC_UTF16 | USING_STRING | NO_CONTEXT | EMBED_ABLE | EMBED_AFTER_NEW;
  hp.embed_hook_font = F_TextOutW | F_GetTextExtentPoint32W;
  hp.filter_fun = BGI7Filter;
  hp.offset = get_reg(regs::rax);
  static uintptr_t replaceaddr = addr;
  patch_fun = []()
  {
    ReplaceFunction((void *)replaceaddr, +[](LPCCH lpMultiByteStr)
                                         { return allocateString(StringToWideString(lpMultiByteStr, 932).value_or(L"")); });
  };
  return NewHook(hp, "BGI");
}
bool BGI::attach_function()
{
  return BGIattach_function2() || BGIattach_function1();
}