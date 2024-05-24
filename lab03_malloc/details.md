# IMPLICIT FREELIST: Implementation Details

`Block Structure`
* Header + Payload + Footer
* Header: block size와 allocated 여부 저장
    - 8byte aligned이므로 최하위 3bit를 alloc 여부 저장에 사용

`Placing Policy`
* alloc 요청된 block을 어디에 할당할 것인가?
* first fit, next fit, best fit
* segregated free list에 first fit을 적용하면 best fit에 근사함

`Splitting`
* free block의 일부를 alloc 할 때

`Coalescing`
* 인접한 free block을 하나로 병합하는 것
* 이때 앞쪽에 위치한 block까지 참조하기 위해 footer 필요 (boundary tag)
* 총 4개 case 존재 -> 각각 대응

`Heap Structure`
* 처음과 끝에는 prologue block, epilogue block이 위치
    - prologue block: 8/1 + 8/1이 저장된 2word block
    - epilogue block: 0/1이 저장된 1word block
    - align 위해 맨 앞에 빈 word 하나 추가

---

`realloc`
* 해당 block의 크기를 조정하거나 새로운 공간 할당
* coalesce와 유사하게 앞뒤 free block을 조사해 크기가 충분하면 크기 조정 후 할당
* 그런데 앞 block과 병합하면 어차피 데이터를 이동시켜야 함
* 뒤 block이 free인 경우에만 size를 검사해 크기 조정하고, 그렇지 않으면 free 후 다시 malloc 수행
* 커지는 경우 나머지 공간은 0으로 초기화해야 함

# Problems
* header와 footer의 정보가 다른 경우 발생
    -> split 시 최소 size 준수하도록

---

* 실행 결과 공간 효율성이 떨어짐
* pop block이 시간을 많이 잡아먹음 (왜지?)
* alloc 되는 블록의 크기가 커서 작은 쪽 size set은 거의 쓰이지 않음
    - set limit 개편 필요
    - 기준을 2^(n+5)로 바꿨더니 util 1점 상승

* realloc에서 뒤 block이 free인 경우만 고려했음
    - 앞 block이 free인 경우까지 고려하도록 수정? -> 가성비 떨어지는듯
* 새로운 block을 찾을 때 free 후 malloc 하도록 수정해야 함
    - 메모리 내용을 복사해둘 임시 공간 필요
    -> thru가 떨어져서 오히려 총점 하락