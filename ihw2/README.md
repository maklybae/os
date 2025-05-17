# ИДЗ-2

*Клычков Максим Дмитриевич. БПИ237. Вариант 22.*

## Текст задания

> **Задача о картинной галерее**. Вахтер следит за тем, чтобы в
картинной галерее одновременно было не более 25 посетителей.
Для обозрения представлены 5 картин. Каждый посетитель случайно переходит от картины к картине,
но если на желаемую картину любуются более пяти посетителей, он стоит в стороне и ждет,
пока число желающих увидеть эту картину не станет меньше. Посетитель покидает галерею по завершении осмотра всех картин.
Каждый посетитель уникальный, то есть имеет свой номер (например, PID). В галерею также пытаются постоянно зайти новые
посетители, которые ожидают своей очереди и разрешения от вахтера, если та заполнена.

> **Создать многопроцессное приложение, моделирующее однодневную работу картинной галереи (например, можно ограничить числом посетителей от 100 до 300).**

> *Вахтер и посетители — отдельные процессы*.

## Общая схема решения

Предлагается использовать 5 семафоров, которые изначально инициализируются значением 5, то есть эти семафоры отвечают за количество наблюдателей конкретной картины. Также используется один семафор, отвечающий за количество людей в галерее, который изначально инициализируется значением 25.

Возникает вопрос о роли **вахтера** в такой модели. Действительно, новые посетители не смогут пройти в галерею, пока она заполнена (в ней уже 25 посетителей). Посмотрев все картины посетитель покидает галерею, освобождая место (семафор) для нового посетителя.

Совместно с семинаристом было принято решение о том, что процесс-вахтер занимается тем, что открывает картинную галерею (инициализирует все семафоры), а также закрывает ее по своему решению (ожидает сигнала SIGINT, освобождает и удаляет семафоры). Также в нескольких программах (на некоторые оценки) создает процессы-посетители, которые будут пытаться зайти в галерею и посмотреть картины.

Что касается логики обхода картин посетителем, то она заключается в следующем: вошедший в галерею посетитель выбирает случайную картину и пытается ее посмотреть. Если на картину уже смотрят 5 человек, то он ждет, пока кто-то не уйдет. После этого он смотрит картину (случайное время от 1 до 5 секунд), затем отходит от нее, освобождая место для нового смотрящего, выбирает другую, еще не осмотренную картину и повторяет процесс. После того, как все картины были осмотрены, он покидает галерею, освобождая место для нового посетителя.

## 4-5 баллов

### Общая схема решения

Реализуем программу с помощью множества процессов, взаимодействующих с использованием **именованных** POSIX семафоров. Из данных необходимо обмениваться только семафорами, но так как процессы-посетители запускаются от родительского процесса-вахтера, то в памяти уже находится информация о семафорах, которые были созданы в родительском процессе.

Сейчас количество посетителей будет ограничено числом, переданным через аргументы командной строки. Также в программе будет реализована возможность передачи сигнала SIGINT, который будет отвечать за завершение работы программы, при этом посетитель досматривает последнюю картину, а затем покидает галерею. Вахтер завершает работу, освобождая семафоры и удаляя их.

### Логика решения задачи

Процесс родитель инициализирует все семафоры — начало рабочего дня картинной галереи.

Затем он создает процессы-посетители, которые будут пытаться зайти в галерею и посмотреть картины.

```c
// Visitor process

// Init random number generator (use pid as seed to avoid same seed for
// all visitors)
srand(getpid());

// Wait for gallery semaphore
if (sem_wait(gallery_sem) == -1) {
  perror("sem_wait");
  return 1;
}
printf("Visitor %d entered the gallery\n", getpid());

uint8_t visited = 0;
while (visited != ALL_PAINTINGS && !finish_flag) {
  // Randomly select a painting to view
  // Ensure that the selected painting has not been viewed yet
  uint8_t next_painting;
  while (1 << (next_painting = rand() % PAINTING_COUNT) & visited) {
  }

  // Wait for painting semaphore
  if (sem_wait(painting_sems[next_painting]) == -1) {
    perror("sem_wait");
    return 1;
  }

  printf("Visitor %d is viewing painting %d\n", getpid(),
          next_painting + 1);
  sleep(rand() % (MAX_WAITING_TIME - MIN_WAITING_TIME + 1) +
        MIN_WAITING_TIME);
  visited |= 1 << next_painting;

  // Signal painting semaphore
  if (sem_post(painting_sems[next_painting]) == -1) {
    perror("sem_post");
    return 1;
  }
  printf("Visitor %d finished viewing painting %d\n", getpid(),
          next_painting + 1);
}

// Signal gallery semaphore
if (sem_post(gallery_sem) == -1) {
  perror("sem_post");
  return 1;
}
printf("Visitor %d left the gallery\n", getpid());

// Exit visitor process
return 0;
```

Заметим, что для каждого посетителя заводится переменная `visited`, которая отвечает за то, какие картины он уже посмотрел. Она инициализируется нулем, а затем при просмотре картины обновляется с помощью побитового ИЛИ. Другими словами, используются битовые маски.

```c
enum PaintingFlags {
  PAINTING_1 = 1 << 0, // 00001
  PAINTING_2 = 1 << 1, // 00010
  PAINTING_3 = 1 << 2, // 00100
  PAINTING_4 = 1 << 3, // 01000
  PAINTING_5 = 1 << 4, // 10000
  ALL_PAINTINGS = 0x1F // 11111
};
```

Также для каждого процесса-посетителя создается свой генератор случайных чисел, чтобы избежать одинаковых последовательностей случайных чисел.

### Вывод программы

```bash
$ ./a.out 5
```

```
Working day started...
Visitor 6993 entered the gallery
Visitor 6993 is viewing painting 2
Visitor 6994 entered the gallery
Visitor 6994 is viewing painting 4
Visitor 6995 entered the gallery
Visitor 6995 is viewing painting 1
Visitor 6996 entered the gallery
Visitor 6996 is viewing painting 3
Visitor 6997 entered the gallery
Visitor 6997 is viewing painting 5
Visitor 6995 finished viewing painting 1
Visitor 6995 is viewing painting 5
Visitor 6994 finished viewing painting 4
Visitor 6994 is viewing painting 5
Visitor 6997 finished viewing painting 5
Visitor 6997 is viewing painting 4
Visitor 6993 finished viewing painting 2
Visitor 6993 is viewing painting 4
Visitor 6996 finished viewing painting 3
Visitor 6996 is viewing painting 2
Visitor 6995 finished viewing painting 5
Visitor 6995 is viewing painting 4
Visitor 6993 finished viewing painting 4
Visitor 6993 is viewing painting 1
Visitor 6997 finished viewing painting 4
Visitor 6997 is viewing painting 1
Visitor 6997 finished viewing painting 1
Visitor 6997 is viewing painting 2
Visitor 6994 finished viewing painting 5
Visitor 6994 is viewing painting 2
Visitor 6996 finished viewing painting 2
Visitor 6996 is viewing painting 1
Visitor 6995 finished viewing painting 4
Visitor 6995 is viewing painting 2
Visitor 6997 finished viewing painting 2
Visitor 6997 is viewing painting 3
Visitor 6996 finished viewing painting 1
Visitor 6996 is viewing painting 5
Visitor 6997 finished viewing painting 3
Visitor 6997 left the gallery
Visitor 6993 finished viewing painting 1
Visitor 6993 is viewing painting 5
Visitor 6994 finished viewing painting 2
Visitor 6994 is viewing painting 3
Visitor 6993 finished viewing painting 5
Visitor 6993 is viewing painting 3
Visitor 6996 finished viewing painting 5
Visitor 6996 is viewing painting 4
Visitor 6995 finished viewing painting 2
Visitor 6995 is viewing painting 3
Visitor 6993 finished viewing painting 3
Visitor 6993 left the gallery
Visitor 6994 finished viewing painting 3
Visitor 6994 is viewing painting 1
Visitor 6996 finished viewing painting 4
Visitor 6996 left the gallery
Visitor 6994 finished viewing painting 1
Visitor 6994 left the gallery
Visitor 6995 finished viewing painting 3
Visitor 6995 left the gallery
Working day finished...
```

## 6-7 баллов

Реализуем программу с помощью множества процессов, взаимодействующих с использованием **неименованных** POSIX семафоров. Из данных необходимо обмениваться только семафорами, но так как процессы-посетители запускаются от родительского процесса-вахтера, то в памяти уже находится информация о семафорах, которые были созданы в родительском процессе.

По сути вся логика остается прежней, но теперь используются **неименованные** семафоры.

## Изменения

Покажем изменения в коде, которые были внесены для работы с неименованными семафорами. Изменения касаются только инициализации семафоров и их удаления. Также был изменен тип переменной-семафора с `sem_t*` на `sem_t`.

Инициализация семафоров:

```c
// Inint gallery semaphore
if (sem_init(&gallery_sem, true, MAX_VISITORS) == -1) {
  perror("sem_init");
  return 1;
};

// Initialize painting semaphores (stored in array)
for (int i = 0; i < PAINTING_COUNT; ++i) {
  if (sem_init(&painting_sems[i], true, MAX_VIEWERS) == -1) {
    perror("sem_init");

    // Clean up only successfully created semaphores
    CleanUp(i);
    return 1;
  }
}
```
Удаление семафоров:

```c
void CleanUp(int paintings) {
  // Close and unlink semaphores
  if (sem_destroy(&gallery_sem) == -1) {
    perror("sem_destroy");
  }
  for (int i = 0; i < PAINTING_COUNT; ++i) {
    if (sem_destroy(&painting_sems[i]) == -1) {
      perror("sem_destroy");
    }
  }
}
```

В остальном код остался прежним. 

## 8 баллов

Множество независимых процессов взаимодействуют с использованием семафоров в стандарте UNIX SYSTEM V. Обмен данными ведется через разделяемую память в стандарте UNIX SYSTEM V.

### Общая схема решения

Явно пропишем, какие программы за что будут отвечать.

1. **Процесс-вахтер** — отвечает за создание семафоров и разделяемой памяти, а также за их удаление. Он ждет сигнала SIGINT, после чего сообщает процессу-посетителю о том, что картинная галерея закрывается (в этом варианте через разделяемую память UNIX SYSTEM V), завершает работу, освобождая семафоры и удаляя их.
2. **Процесс-посетитель** входит в галерею, выбирает случайную картину и пытается ее посмотреть. При выходе из галереи и освобождении семафора процесс завершает свою работу.

### Разделяемая память

В разделяемой память хранится два флага:

1. `finish_flag` — отвечает за то, что галерея закрыта и посетители должны покинуть ее.
2. `ready` — отвечает за то, что разделяемая память и семафоры проинициализированы, то есть открылась картинная галерея.

### Процесс-вахтер

Работа с семафорами происходит следующим образом:

```c
// Initialize semaphores
unsigned short values[] = {MAX_VISITORS, MAX_VIEWERS, MAX_VIEWERS,
                            MAX_VIEWERS,  MAX_VIEWERS, MAX_VIEWERS};
if (semctl(sem_id, 0, SETALL, values) < 0) {
  perror("semctl SETALL");
  return 1;
}

// Set ready flag (gallery is open)
shmem->ready = 1;
printf("Working day started...\n");

// Waiting for close signal
while (!shmem->finish_flag) {
  sched_yield();
}
```

Используется `sched_yield()`, чтобы не нагружать процессор busy-loop. Важно отметить, что семафоры и разделяемая память создаются в одном процессе, а затем используются в других процессах. Другого интересного в этом процессе нет.

### Процесс-посетитель

В процессе-посетителе повторяется логика посетителя из предыдущих пунктов, при этом используется вся типичная для UNIX SYSTEM V логика работы с семафорами и разделяемой памятью.

### Тестирование

Для тестирования программы использовался [скрипт](8/test.sh), который запускает сначала программу вахтер, а затем 50 посетителей. Логи можно посмотреть в [файле](8/test_result.txt).

По поиску по файлу можно обнаружить, что заходит 50 человек, выходит 50 человек. По поиску по конкретному PID можно посмотреть, что каждый посетитель посетитель посмотрел все картины.

## 9 баллов

Реализуем программу на множестве независимых процессов взаимодействующих с использованием семафоров в стандарте UNIX SYSTEM V. Обмен данными ведется через через очереди сообщений в стандарте UNIX SYSTEM V.

Эта программа будет отличаться от программы на 8 баллов только тем, что нужно использовать каналы в стандарте UNIX SYSTEM V вместо разделяемой памяти.

### Общая схема решения

Не совсем понятно, как к нынешней программе привязать очереди сообщений, кажется, что любое использование каналов здесь немного неуместно. Будем реализовывать следующее:

Пусть ожидание посетителей в галерее будет происходить на стороне посетителя, то есть он будет ждать, пока в галерее не освободится место. После этого он сможет зайти в галерею и посмотреть картины. После того, как он посмотрел все картины, он сообщает вахтеру через очередь сообщений, что закончил осмотр всех картин и покидает галерею (завершается процесс). Вахтер принимает информацию о том, что посетитель закончил осмотр и увеличивает значение семафора (post), отвечающего за количество людей в галерее.

Были выбраны именно очереди сообщений, так как по сравнению с pipe они имеют встроенную структуру сообщений (не просто поток байтов), а также поддерживают многие-ко-одному коммуникацию.

### Процесс-вахтер

Содержательное изменение в процессе-вахтере:

```c
// Process visitor completion messages until finish signal
visitor_msg_t message;
while (!finish_flag) {
  // Non-blocking message receive
  int result = msgrcv(msg_id, &message, sizeof(visitor_msg_t) - sizeof(long),
                      MSG_VISITOR_FINISHED, IPC_NOWAIT);

  if (result < 0) {
    if (errno == ENOMSG) {
      // No message available, just wait a bit
      usleep(100000); // 100ms
      continue;
    } else if (errno == EINTR) {
      // Interrupted by signal
      continue;
    } else {
      perror("msgrcv");
      CleanUp();
      return 1;
    }
  }

  // Process visitor completion message
  printf("Watchman: Visitor %d completed viewing all paintings\n",
          message.pid);

  // Signal gallery semaphore to allow another visitor
  struct sembuf leave_arg[1] = {{.sem_num = 0, .sem_op = 1, .sem_flg = 0}};
  if (semop(sem_id, leave_arg, 1) < 0) {
    if (errno != EINTR) {
      perror("semop (gallery release)");
      CleanUp();
      return 1;
    }
  }
}
```

В этом коде вахтер ждет сообщения от посетителей о том, что они закончили осмотр всех картин. После этого он увеличивает значение семафора, отвечающего за количество людей в галерее.

### Процесс-посетитель

Процесс-посетитель, когда он закончил осмотр всех картин, отправляет сообщение вахтеру о том, что он закончил осмотр. После этого он завершает свою работу.

```c
if (visited == ALL_PAINTINGS) {
  // Send message to watchman that visitor is leaving
  visitor_msg_t message;
  message.mtype = MSG_VISITOR_FINISHED;
  message.pid = getpid();

  if (msgsnd(msg_id, &message, sizeof(visitor_msg_t) - sizeof(long), 0) < 0) {
    perror("msgsnd");
    return 1;
  }

  printf("Visitor %d has seen all paintings and left the gallery\n",
          getpid());
} else {
  // If interrupted before seeing all paintings, still need to free the
  // semaphore
  struct sembuf leave_arg[1] = {{.sem_num = 0, .sem_op = 1, .sem_flg = 0}};
  if (semop(sem_id, leave_arg, 1) < 0 && errno != EINTR) {
    perror("semop (gallery leave)");
  }
  printf("Visitor %d left the gallery early\n", getpid());
}

return 0;
```

### Тестирование

Для тестирования программы использовался [скрипт](9/test.sh), который запускает сначала программу вахтер, а затем 50 посетителей. Логи можно посмотреть в [файле](9/test_result.txt).

По поиску по файлу можно обнаружить, что заходит 50 человек, выходит 50 человек. По поиску по конкретному PID можно посмотреть, что каждый посетитель посетитель посмотрел все картины.


## 10 баллов

Реализуем программу на множестве независимых процессов, взаимодействующих с использованием POSIX семафоров и POSIX очередей сообщений.

### Общая схема решения

В этой реализации процессы будут взаимодействовать через именованные POSIX семафоры и POSIX очереди сообщений:

1. **Процесс-вахтер** - создает и инициализирует POSIX семафоры и очередь сообщений, обрабатывает сообщения от посетителей и закрывает галерею по сигналу SIGINT.

2. **Процесс-посетитель** - ожидает доступа в галерею (используя семафор), последовательно осматривает картины (используя соответствующие семафоры), а после осмотра всех картин отправляет сообщение вахтеру через очередь сообщений и завершается.

### Изменения по сравнению с программой на прошлую оценку

Изменился лишь интерфейс взаимодействия с семафором и очередью сообщений (с UNIX SYSTEM V переехали на POSIX), никаких других изменений нет.

### Тестирование

Для тестирования программы использовался [скрипт](10/test.sh), который запускает сначала программу вахтер, а затем 50 посетителей. Логи можно посмотреть в [файле](10/test_result.txt).

По поиску по файлу можно обнаружить, что заходит 50 человек, выходит 50 человек. По поиску по конкретному PID можно посмотреть, что каждый посетитель посетитель посмотрел все картины.
