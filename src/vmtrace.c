/*
 * Copyright 2016 Andrei Pangin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jvmti.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

static jlong start_time;

static void trace(jvmtiEnv *jvmti_env, const char* fmt, ...) {
    jlong current_time;
    (*jvmti_env)->GetTime(jvmti_env, &current_time);

    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    jlong time = current_time - start_time;
    fprintf(stderr, "[%d.%05d] %s\n", (int)(time / 1000000000),
                                      (int)(time % 1000000000 / 10000), buf);
}

static char* fix_class_name(char* class_name) {
    // Strip 'L' and ';' from class signature
    class_name[strlen(class_name) - 1] = 0;
    return class_name + 1;
}

void JNICALL VMStart(jvmtiEnv *jvmti_env, JNIEnv* jni_env) {
    trace(jvmti_env, "VM started");
}

void JNICALL VMInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
    trace(jvmti_env, "VM initialized");
}


void JNICALL ClassFileLoadHook(jvmtiEnv *jvmti_env, JNIEnv* jni_env,
                               jclass class_being_redefined, jobject loader,
                               const char* name, jobject protection_domain,
                               jint data_len, const unsigned char* data,
                               jint* new_data_len, unsigned char** new_data) {
    trace(jvmti_env, "Loading class: %s", name);
}

void JNICALL ClassPrepare(jvmtiEnv *jvmti_env, JNIEnv* jni_env,
                          jthread thread, jclass klass) {
    char* name;
    (*jvmti_env)->GetClassSignature(jvmti_env, klass, &name, NULL);
    trace(jvmti_env, "Class prepared: %s", fix_class_name(name));
    (*jvmti_env)->Deallocate(jvmti_env, name);
}

void JNICALL DynamicCodeGenerated(jvmtiEnv *jvmti_env, const char* name,
                                  const void* address, jint length) {
    trace(jvmti_env, "Dynamic code generated: %s", name);
}

void JNICALL CompiledMethodLoad(jvmtiEnv *jvmti_env, jmethodID method,
                                jint code_size, const void* code_addr,
                                jint map_length, const jvmtiAddrLocationMap* map,
                                const void* compile_info) {
    jclass holder;
    char* holder_name;
    char* method_name;
    (*jvmti_env)->GetMethodName(jvmti_env, method, &method_name, NULL, NULL);
    (*jvmti_env)->GetMethodDeclaringClass(jvmti_env, method, &holder);
    (*jvmti_env)->GetClassSignature(jvmti_env, holder, &holder_name, NULL);
    trace(jvmti_env, "Method compiled: %s.%s", fix_class_name(holder_name), method_name);
    (*jvmti_env)->Deallocate(jvmti_env, method_name);
    (*jvmti_env)->Deallocate(jvmti_env, holder_name);
}

void JNICALL GarbageCollectionStart(jvmtiEnv *jvmti_env) {
    trace(jvmti_env, "GC started");
}

void JNICALL GarbageCollectionFinish(jvmtiEnv *jvmti_env) {
    trace(jvmti_env, "GC finished");
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv* jvmti;
    (*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_0);

    (*jvmti)->GetTime(jvmti, &start_time);
    trace(jvmti, "VMTrace started");

    jvmtiCapabilities capabilities = {0};
    capabilities.can_generate_all_class_hook_events = 1;
    capabilities.can_generate_compiled_method_load_events = 1;
    capabilities.can_generate_garbage_collection_events = 1;
    (*jvmti)->AddCapabilities(jvmti, &capabilities);
    
    jvmtiEventCallbacks callbacks = {0};
    callbacks.VMStart = VMStart;
    callbacks.VMInit = VMInit;
    callbacks.ClassFileLoadHook = ClassFileLoadHook;
    callbacks.ClassPrepare = ClassPrepare;
    callbacks.DynamicCodeGenerated = DynamicCodeGenerated;
    callbacks.CompiledMethodLoad = CompiledMethodLoad;
    callbacks.GarbageCollectionStart = GarbageCollectionStart;
    callbacks.GarbageCollectionFinish = GarbageCollectionFinish;
    (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));

    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_START, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_DYNAMIC_CODE_GENERATED, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);

    return 0;
}
