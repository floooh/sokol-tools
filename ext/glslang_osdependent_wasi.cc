// dummy stubs for platforms that don't support threading

namespace glslang {

static void* tls_value;

typedef void* OS_TLSIndex;

OS_TLSIndex OS_AllocTLSIndex() {
    return (void*)&tls_value;
}

bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue) {
    (void)nIndex;
    tls_value = lpvValue;
    return true;
}
bool OS_FreeTLSIndex(OS_TLSIndex nIndex) {
    (void)nIndex;
    return true;
}

void* OS_GetTLSValue(OS_TLSIndex nIndex) {
    (void)nIndex;
    return tls_value;
}

void InitGlobalLock() {}
void GetGlobalLock() {}
void ReleaseGlobalLock() {}

}