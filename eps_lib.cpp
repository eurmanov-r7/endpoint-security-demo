#include "eps_lib.h"
#include <bsm/libbsm.h>
#include <EndpointSecurity/EndpointSecurity.h>
#include <iostream>
#include <queue>
#include <deque>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

std::queue<struct proc_start_payload> g_q;
std::mutex g_q_mutex;
bool g_stop = false;

struct proc_start_payload build_proc_start_payload(const es_message_t *msg) {
    struct proc_start_payload payload;
    strcpy(payload.exe_path, msg->process->executable->path.data);  // const char * data;
    payload.pid = audit_token_to_pid(msg->process->audit_token);
    payload.ppid = msg->process->ppid;
    return payload;
}

struct proc_start_payload read_from_global_struct_queue() {
    g_q_mutex.lock();
    struct proc_start_payload p = g_q.front();
    g_q.pop();
    g_q_mutex.unlock();
    return p;
}

static void handle_and_enqueue_event(const es_message_t *msg) {
    struct proc_start_payload payload;
    switch (msg->event_type) {
        case ES_EVENT_TYPE_NOTIFY_EXEC:
            printf("ES_EVENT_TYPE_NOTIFY_EXEC\n");
            payload = build_proc_start_payload(msg); 
            g_q_mutex.lock();
            g_q.push(payload);
            g_q_mutex.unlock();
            printf("msg->process->start_time: %d\n", msg->process->start_time);
            printf("msg->event.exec.target->start_time: %d\n", msg->event.exec.target->start_time);
            printf("msg->event.exec.target->executable->path.data: %s\n", msg->event.exec.target->executable->path.data);
            printf("arg count: %d\n", es_exec_arg_count(&msg->event.exec));
            printf("arg0: %s\n", es_exec_arg(&msg->event.exec, 0).data);
            uint32_t argc;
            argc = es_exec_arg_count(&msg->event.exec);
            for (int i = 0; i < argc; i++) {
//                cout << i << "\n";
                printf("arg %d: %s\n", i, es_exec_arg(&msg->event.exec, i).data);
            }
//            strcpy(payload.exe_path, msg->process->executable->path.data);  // const char * data;
//            payload.pid = audit_token_to_pid(msg->process->audit_token);
//            payload.ppid = msg->process->ppid;
//            printf("msg->event.exit.stat: %s\n", msg->event.exit.stat);  This will break!
            break;

        case ES_EVENT_TYPE_AUTH_EXEC:
            printf("ES_EVENT_TYPE_AUTH_EXEC\n");
            break;

        case ES_EVENT_TYPE_NOTIFY_AUTHENTICATION:
            printf("\n\n\nES_EVENT_TYPE_NOTIFY_AUTHENTICATION\n");
            printf("msg->event.authentication->type: %d\n", msg -> event.authentication->type);
            printf("msg->time: %d\n", msg->time); // Note that msg->process->start_time isn't what we want!
            printf("msg->event.authentication->success: %d\n", msg -> event.authentication -> success);
            printf("msg->event.authentication->data.token->token_id: %s\n", msg -> event.authentication->data.token -> token_id.data); //Looks like the authenticating user?

            break;

        // case ES_EVENT_TYPE_NOTIFY_FORK:
        //     printf("%{public}s (pid: %d) | FORK: Child pid: %d",
        //         msg->process->executable->path.data,
        //         audit_token_to_pid(msg->process->audit_token),
        //         audit_token_to_pid(msg->event.fork.child->audit_token));

        //     break;

        // case ES_EVENT_TYPE_NOTIFY_EXIT:
        //     printf("%{public}s (pid: %d) | EXIT: status: %d",
        //         msg->process->executable->path.data,
        //         audit_token_to_pid(msg->process->audit_token),
        //         msg->event.exit.stat);
        //     break;

         default:
             printf("%d\n", msg->event_type);
             break;
    }
}

void start_handling_events() {
    es_client_t *client;

    es_new_client_result_t result = es_new_client(&client, ^(es_client_t *c, const es_message_t *msg) {
        handle_and_enqueue_event(msg);
    });
    if (result != ES_NEW_CLIENT_RESULT_SUCCESS) {
        printf("Failed to create new ES client: %d", result);
        return;
    }

    es_event_type_t events[] = { ES_EVENT_TYPE_NOTIFY_EXEC, ES_EVENT_TYPE_NOTIFY_FORK, ES_EVENT_TYPE_NOTIFY_EXIT, ES_EVENT_TYPE_NOTIFY_AUTHENTICATION };

    if (es_subscribe(client, events, sizeof(events) / sizeof(events[0])) != ES_RETURN_SUCCESS) {
        printf("Failed to subscribe to events");
        es_delete_client(client);
        return;
    }

    while (g_stop == false);
    printf("Deleting client and stopping program \n");
    es_delete_client(client);
    return;
}

void report_metrics() {
    while (g_stop == false) {
        std::cout << "Event Que Size: " << g_q.size() << std::endl;
        sleep(5);
    }
}

void shutdown() {
    g_stop = true;
}
