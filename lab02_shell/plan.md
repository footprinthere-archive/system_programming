# Job controls
`addjob(job_t*, pid, state, cmdline)`
    - 새로운 작업 추가
`deletejob(job_t*, pid)`
    - PID 일치하는 작업 제거
`fgpid(jobs)`
    - fg job의 PID 반환, 없으면 0
`getjobpid(jobs, pid)`
    - PID가 일치하는 job의 pointer 반환
`getjobjid(jobs, jid)`
    - JID가 일치하는 job의 pointer 반환
`pid2jid(pid)`
    - PID를 입력 받아 JID 반환


# Problems
* execve 할 때 command의 validity를 검사하게 되는데, addjob 등 작업은 그 후에 이루어져야 함
    * `pipe` or shared memory?
    * shared memory에 성공 정보 저장? -> 저장이 먼저 실행될지, 값 체크가 먼저 실행될지 undefined
    * child에서 deletejob() 하기? -> 어차피 jobs가 따로 생성되므로 parent에 영향을 주지 못함

* Ctrl+Z를 받아 정지되었을 때, waitfg()를 탈출할 수 있도록 해야 함 -> 해결
* n_bgproc 필요한가? (필요 없으면 제거할 것) -> 제거함

* `myint는 자기 자신에게 SIGINT를 보내는데, shell에서 catch가 안 됨`
    * 원래 child에게 send 된 signal을 parent에서 catch 할 수 있는 게 맞나?

* Ctrl+C, Ctrl+Z 아예 안 먹힘
* background도 foreground처럼 실행됨
    * child에서 sleep이 실행될 때 parent도 함께 멈춰버림
    * pipe 때문인가?

# TODO
* 모든 system call의 output 체크하기