메모리 배치 설계

Page Directory
 - Page에 대한 reference
   - Page 시작 offset
   - 해당 Page에 남은 free byte의 수


Page
 - # of records
 - free spaces
 - next/previous pointer to logically next/previous page
 - slots
 - bitmap

설계시 변경사항

1. key에 해당하는 자료형을 int에서 uint_64로 바꿔야 함.
2. 