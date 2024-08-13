#include <stdio.h>
#include <string.h>
#include <windows.h>


// 상수 정의
#define PROCESS_MAX_NUM 10000
#define MAX_TIME 1000
#define TERMINATION_FLAG 1


// 함수 선언
void create_processes();
int id_check(int id);
void pcb_init(int id, int execution_time, int creation_time);

void process_ready_check(int time);
void process_ready(int i, int t);
int process_termination_check(int index);
int  process_terminate(int time, int index, int flag);

void print_processes();
void round_robin();
int context_switch(int time, int current_index, int flag);


// PCB 구조체 정의
typedef struct pcb {
	int id;
	int execution_time;
	int creation_time;
	int remain_time;
} PCB;


// 전역 변수 선언
int pcb_count = 0;
int pcb_ready_count = 0;

PCB pcb_list[PROCESS_MAX_NUM];
PCB pcb_ready_list[PROCESS_MAX_NUM];


// events.txt 파일로부터 Process를 생성하는 함수
void create_processes()
{
	FILE* f;
	int creation_time;
	int execution_time;
	int id;

	fopen_s(&f, "events.txt", "r");

	int flag = 0;

	do {
		fscanf_s(f, "%d", &creation_time);

		if (creation_time == -1) { // .txt파일을 읽다가 -1을 만나는 경우
			flag = 1; // .txt파일을 다 읽은 경우 flag = 1
			fclose(f);
		}
		else {
			fscanf_s(f, "%d", &id);
			fscanf_s(f, "%d", &execution_time);


			int error_check = id_check(id); 
// 중복되는 PID 있는지 확인. 
//중복되면 해당 id를 return, 중복되는 id가 없으면 -1을 return

			if (error_check == -1) {
				pcb_init(id, execution_time, creation_time);
			}
			else {
				printf("입력한 Process ID  %d 중복. \n", id);
			} // 중복된 PID 없으면 PCB 생성, 중복되면 error 출력
		}

	} while (flag == 0); // .txt 파일을 다 읽어 flag가 1이 되기 전까지 반복

	printf("\n");
}


// Process ID 체크 함수 -- process_create()와 process_terminate()에 사용
int id_check(int id)
{
	for (int i = 0; i < pcb_count; i++) {
		if (pcb_list[i].id == id) {
			return i; // 동일한 id의 index를 만나면 해당 index를 return
		}
	}
	return -1; // 동일한 id가 없으면 -1을 return
}


// PCB 초기화 함수
void pcb_init(int id, int execution_time, int creation_time)
{
	PCB pcb;

	pcb.id = id;
	pcb.execution_time = execution_time;
	pcb.creation_time = creation_time;
	pcb.remain_time = execution_time;

	pcb_list[pcb_count] = pcb;

	pcb_count++;
}


// 특정 time에 ready되는 process를 체크하는 함수
// creation time = 현재 time인 pcb가 있으면 ready상태로 전환
void  process_ready_check(int time)
{
	int t = time;

	for (int i = 0; i < pcb_count; i++) {
		if (pcb_list[i].creation_time == time) {
			process_ready(i, t);
		}
	}
}


// ready되는 process를 pcb_ready_list에 연결하는 함수
// pcb_list에 만들어둔 pcb를 pcb_ready_list에 복사 
void process_ready(int i, int t)
{
	int time = t;
	memcpy(&pcb_ready_list[pcb_ready_count], &pcb_list[i], sizeof(pcb_list[i]));
	pcb_ready_count++;

	printf("현재 시간 %d초. Process ID %d 생성. \n", time, pcb_list[i].id);
}


// 실행중인 process가 종료되어야 하는지 체크하는 함수
// remain_time = 0 이면 flag = TERMINATION_FLAG를 return
int  process_termination_check(int index)
{
	int i = index;
	if (pcb_ready_list[i].remain_time == 0) {

		return TERMINATION_FLAG;
	}
	return 0;
}


// 종료되는 process를 list들에서 제거하는 함수
// pcb_list와 pcb_ready_list 두 list에서 모두 제거해야 함
// 제거하기 전에 context switch 먼저 수행 (context switch하면 index가 변경되기 떄문)
// context switch 수행 후 list들에서 pcb 제거, count상수들도 감소
int  process_terminate(int time, int index, int flag)
{
	int t = time;
	int index1 = index; // 제거할 process의 pcb_ready_list상의 index
	int id = pcb_ready_list[index1].id;
	int index2 = id_check(id); // 제거할 process의 pcb_list상의 index
	int f = flag;

	int next_index = context_switch(time, index1, f); // Context Switch

	for (; index2 < pcb_count; index2++) {
		pcb_list[index2] = pcb_list[index2 + 1];
	}
	pcb_count--; // pcb_list에서 process 제거

	for (; index1 < pcb_ready_count; index1++) {
		pcb_ready_list[index1] = pcb_ready_list[index1 + 1];
	}
	pcb_ready_count--; // pcb_ready_list에서 process 제거

	return next_index; // 제거 후 실행되어야 할 index를 return
}


// round-robin scheduling 함수
void round_robin()
{
	int time = 0;
	int time_quantum;
	int time_quantum_count;
	int current_index = -1; 
// 실행중인 process의 pcb_ready_list상의 index. 
// 실행할 process가 없는 초기 상태는 -1
	int flag = 0; 
// context switch의 원인이 time_quantum 만료 or process 종료 인지 나타내는 flag

	printf("round-robin scheduling의 time quantum을 입력하세요 : ");
	scanf_s("%d", &time_quantum); // time quantum 입력받기
	printf("\n");
	time_quantum_count = time_quantum; // time quantum count 초기화

	while (time != MAX_TIME) {

		process_ready_check(time); 
// 해당 time에 ready상태로 전환할 process가 있는지 체크

		if (current_index == -1) { // 기존에 아무 process도 실행중이지 않은 경우
			if (pcb_ready_count != 0) { // 새로운 process가 생성되어 ready가 되는 경우
				current_index = context_switch(time, current_index, flag); 
				// Context Switch
			}
			// running process가 없고, ready상태가 되는 process도 없으면 바로 time++;
		}
		else { // 기존에 running중인 process가 있는 경우

			time_quantum_count--;
			pcb_ready_list[current_index].remain_time--;

			int flag = process_termination_check(current_index); 
			// 실행중인 process가 종료되는지 체크

			if (flag == TERMINATION_FLAG) { 
			// 실행중인 process가 remain_time이 만료되어 종료되는 경우
				current_index = process_terminate(time, current_index, flag); 
			// 종료하는 process를 list에서 제거 및 바로 context switch
				time_quantum_count = time_quantum; // time quantum count도 초기화
				flag = 0;
			}
			else if ((time_quantum_count == 0) && (pcb_ready_count != 0)) { 
			// 실행중인 process가 time quantum이 만료되는 경우
				current_index = context_switch(time, current_index, flag); 
				// Context Switch
				time_quantum_count = time_quantum; // time quantum count도 초기화
			}
		}

		Sleep(1000); // while문 반복마다 실제 시간 1초가 흐르도록 해주는 함수

		time++; // 실제 시간 1초가 흐를 때 마다 time++
	}

}


// context switch하는 함수
int context_switch(int time, int current_index, int flag)
{
	int t = time; // 현재 시간
	int index = current_index; // 기존 실행중이던 process의 index
	int f = flag;

	if (f == 0) { // process가 ready 혹은 time_quantum이 만료되어 context switch되는 경우
		if (index == -1) { // 실행중이던 process가 없는 경우
			index++; // index 증가시켜 다음 process 실행
			printf("현재 시간 %d초. 이전 process 없음. 다음 process ID : %d. \n",
						 t, pcb_ready_list[index].id);
		}
		else { // 실행중이던 process가 있는 경우
			printf("현재 시간 %d초. 이전 process ID : %d. ", t, pcb_ready_list[index].id);
			index++; // index 증가시켜 다음 process 실행
			if (index >= pcb_ready_count) { 
			// index가 최대 index를 벗어날 경우 index=0 부터 다시 스케줄링 시작
				index = 0;
			}
			printf("다음 process ID : %d. \n", pcb_ready_list[index].id);

		}
	}
	else if (f == TERMINATION_FLAG) { // process가 종료되어 context switch되는 경우
		if ((index == pcb_ready_count - 1) && (pcb_ready_count == 1)) { 
		// 해당 process가 종료되면 더이상 실행할 process가 없는 경우
			printf("현재 시간 %d초. Process ID %d 종료. 다음 process 없음\n",
						 t, pcb_ready_list[index].id);
			index = -1;

		}
		else if ((index == pcb_ready_count - 1) && (pcb_ready_count > 1)) { 
		// process가 종료되고 index=0인 process로 돌아가는 경우
			printf("현재 시간 %d초. Process ID %d 종료. 다음 process ID : %d\n",
						 t, pcb_ready_list[index].id, pcb_ready_list[0].id);
			index = 0;

		}
		else if (index < pcb_ready_count - 1) { 
		// process가 종료되고 다음 index의 process가 실행되는 경우
			printf("현재 시간 %d초. Process ID %d 종료. 다음 process ID : %d\n",
						 t, pcb_ready_list[index].id, pcb_ready_list[index + 1].id);
		}
	}
	return index;
}


// events.txt에 의해 생성된 process를 print하는 함수
void  print_processes()
{
	for (int i = 0; i < pcb_count; i++) {
		int id = pcb_list[i].id;
		int creation_time = pcb_list[i].creation_time;
		int execution_time = pcb_list[i].execution_time;

		printf("%d번째 : Process ID %d, 생성시간 %d, 실행시간 %d. \n",
					 i, id, creation_time, execution_time);
	}
	printf("\n");
}


// main 함수
int main(void)
{
	create_processes();

	print_processes();

	round_robin();

	return 0;
}
