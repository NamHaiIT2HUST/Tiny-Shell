# myTinyShell - A Simple Windows Shell in C

**Ngôn ngữ:** C

**Nền tảng:** Windows

**Môn học:** Nguyên lý Hệ điều hành

---

## Mô tả dự án

`myShell` là một shell đơn giản viết bằng ngôn ngữ C trên Windows. Đây là một bài tập trong môn **Nguyên lý Hệ điều hành**, giúp sinh viên hiểu rõ hơn về:

* Cách tạo và quản lý **tiến trình con** trên Windows (`CreateProcess`, `WaitForSingleObject`).
* Khái niệm **background process** và cách quản lý chúng.
* Các thao tác cơ bản với tiến trình: **dừng, tiếp tục, và kết thúc tiến trình**.
* Quản lý **luồng và handle** trong Windows.

Shell này hỗ trợ cả việc chạy lệnh bình thường hoặc chạy **background** với dấu `&`.

---

## Tính năng

* Chạy các lệnh từ người dùng, bao gồm các file `.bat`.
* Quản lý tiến trình con:

  * `list`: Liệt kê tất cả tiến trình chạy nền, hiển thị PID, tên tiến trình, trạng thái.
  * `kill <pid>`: Kết thúc một tiến trình theo PID.
  * `stop <pid>`: Tạm dừng một tiến trình theo PID.
  * `resume <pid>`: Tiếp tục một tiến trình tạm dừng theo PID.
* Hỗ trợ chạy lệnh **nền (background)** với dấu `&`.
* Lệnh `exit` để thoát shell.

---

## Cách chạy

1. **Compile chương trình**:

```bash
gcc myShell.c -o myShell.exe
```

2. **Chạy chương trình**:

```bash
./myShell.exe
```

3. **Ví dụ sử dụng**:

```
myShell> notepad.exe &
Child process 12345 running in background
myShell> list
PID: 12345, Name: notepad.exe, Status: Running
myShell> stop 12345
Process 12345 stopped
myShell> resume 12345
Process 12345 resumed
myShell> kill 12345
Process 12345 terminated
myShell> exit
Exiting myShell...
```

---

## Cấu trúc code

* `main()`

  * Hiển thị prompt, nhận input từ người dùng.
  * Gọi các hàm `parse_command`, `is_background`, `execute_command`.

* `parse_command()`

  * Tách lệnh thành các tham số (arguments).

* `is_background()`

  * Kiểm tra xem lệnh có chạy nền hay không (`&`).

* `execute_command()`

  * Xử lý các lệnh shell đặc biệt: `exit`, `list`, `kill`, `stop`, `resume`.
  * Tạo tiến trình con sử dụng `CreateProcessA`.
  * Lưu tiến trình con vào `process_list` để quản lý.

* `Process` struct

  * Lưu trữ thông tin tiến trình con: PID, tên, trạng thái, handle.

---

## Lưu ý

* Chỉ hỗ trợ tối đa **100 tiến trình con** cùng lúc.
* Chỉ chạy được trên **Windows** vì sử dụng các hàm Windows API (`CreateProcess`, `TerminateProcess`, `SuspendThread`, `ResumeThread`).
* Khi chạy tiến trình nền, shell không chờ tiến trình hoàn tất.

---

## Tác giả

* Nguyễn Đào Nam Hải (MSSV: 20235321)
