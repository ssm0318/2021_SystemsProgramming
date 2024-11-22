- 오늘은 두번째 실습과제인 쉘랩 설명을 해드리도록 하겠습니다. 
- 먼저 이테일에 있는 스켈레톤 코드 다운 받고, 다운 받은 소스 중 tsh.c를 구현하신 다음에 tsh.c와 레포트를 zip으로 압축해서 제출해주세요.
- 그리고 구현하는 데에 도움이 되는 힌트가 핸드아웃에 많이 있으니 잘 읽어보세요.
- 이번 과제는 4월 14일까지.
- 쉘랩은 process control logic과 signal에 익숙해지자는 목적의 실습.
- 이번 실습과제에서 여러분은 기본적인 job control 기능을 지원하는 간단한 버전의 유닉스 쉘을 구현하게 될 것.
- 4페이지. etl에서 shlab.tar 열어보면 tshref와 trace file들이 있음. trace file을 열어보면 입력으로 들어갈 명령어들을 확인할 수 있음.
- 5페이지로 넘어와서 이번 과제는 기본적으로 제공되는 소스코드가 많으므로 미리 한 번 읽어보시고 시작하시는걸 추천드립니다.
- 그리고 간단한 과제가 아니기 때문에 스스로 헬퍼 함수를 만들어서 사용하기를 권장드립니다.
- sigchild handler 같은 경우 한 번에 여러 child process를 reaping해야할 수 있음을 염두에 두시기 바랍니다. 
- 가장 중요한 것은 trace file로만 디버깅하는 것이 아니라 직접 실행해보고 인풋을 넣어보면서 구현하시는 것을 추천드립니다.
- 6페이지 입니다. 여러분은 eval, builtin command, do background/foreground, wait foreground, sigchild, int, stop handler 총 이렇게 7가지 함수를 구현하시게 됩니다.
- 각 함수의 역할을 슬라이드에 나온 것과 같으니 참고하세요.
- 7페이지를 보시면 어떻게 구현하면 되는지 힌트가 있습니다. 꼭 이렇게 진행하셔야 된다는건 아니지만 어떤걸 먼저 해야할지 감을 잡으실 수 있도록 작성했으니 참고하세요. 
  - 0. parse & check cmd
    1. block signals
    2. create child process
    3. do the job
    4. child process
       1. unblock signal
       2. get new process group ID
       3. load & run new program
    5. parent process
       1. addjob
       2. unblock signal
       3. (if bg) print log message
    6. implement signal handler
- 항상 말씀드리지만 man 페이지를 적극적으로 활용하시기 바랍니다. 이번 과제를 수행하면서 도움이 될만한 함수들을 8페이지에 적어두었으니 한번씩 읽어보시길 바랍니다. 
- 여기서 추가로 tsh 내에서 핸들링 할 시그널들은 eval에서 블로킹해야합니다. 즉, sigchild, sigint, sigstop을 블로킹 해야한다는 것을 염두에 두시기 바랍니다. 
- 작성한 쉘을 테스트할때는 직접 실행해보시길 바랍니다. tsh ref는 쉽게 말해서 답지입니다. 이것의 실행 결과를 보시면서 디버깅하시면 되고요
- 그리고 runtrace로 각 test를 tsh 실행하면서 결과를 확인하실 수 있습니다.
- 10페이지에서는 make한 후 본인의 shell을 실행하는 과정을 보여주고 있음. test case외에 본인이 ref에서는 어떻게 동작하는지 확인하고 싶으시다면 tsh ref를 실행해서 테스트해보시면 됩니다.
- 4페이지에서 말씀드렸던 것처럼 11페이지에서는 trace에서 실행하는 내용이 무엇인지 확인하실 수 있습니다.
- 또 make test하고 번호를 붙여서 실행하면 과제를 진행하면서 잘 구현되어있는지 확인 가능
- rtest와 번호를 입력하면 정답 output도 확인할 수 있으니 참고.
- test 11, 12, 13의 경우 실행할 때마다 아웃풋이 달라질 수 있는데 my split process들의 running state만 확실히 확인할 수 있으면 괜찮습니다.
- 구현하신 것을 test하면서 함께 제공해드린 tsh ref와 완전히 동일한 아웃풋이 나오는지 반드시 확인할 것. 즉 구현하신 것과 tsh ref의 아웃풋을 기록한 txt file이 있으면 diff 명령어를 수행했을 때 정확하게 일치해야함.
- shell driver는 여러분들이 구현하신 tsh를 테스트해주는 프로그램이고 make test 번호로 사용할 수 있음. 아까 알려드린 것처럼.
- 지난 학기에 나왔던 에러들:
  - 맨 위부터 보시면 14페이지에 stop hand부터 해서 대소문자 차이가 나는걸 보실 수가 있는데, 왼쪽 아래 그림은 test 9번을 reference로 실행한 내용. 이와 달리 오른쪽 아래 그림에서는 job을 background로 보냈을 때 shell의 반응이 없으며, 즉 구현에 문제가 있다는 것을 보여줌. 이런 문제가 발생하지 않도록 스스로 테스트 해보면서 염두에 두고 확인하길 바람.
- 15페이지는 예전에 있었던 질문들 정리한 것. 
  - verbose는 구현하지 않아도 되고, jump는 사용하면 안 됨. 
  - system call의 ret는 모두 체크해줘야 하고, my spin?등의 command not found 에러가 발생하면 make를 먼저 수행하셔서 실행을 하시면 됩니다.
- 16페이지에서는 채점과 관련해서 추가로 공지드리겠습니다. 여러분은 trace 파일에 있는 에러 인풋 외에는 다른 에러 인풋이 없다고 가정하고 구현하셔도 무방합니다. 다만, 반드시 모든 부분에 있어서 reference solution과 아웃풋이 완벽하게 동일해야 한다는 점 다시 강조드립니다. 이와 관련한 어떠한 클레임도 추후에 받지 않을 것. 꼭 diff등의 명령어로 확인해볼 것.
- 오늘 과제 설명은 여기까지, 질문 있으면 질문 게시판에 올려주세요!