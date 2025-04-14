Sample codes dealing with these calls are provided. You are also encouraged to read the man pages to understand various options. Finally, Beej's IPC guide linked in the Reference section of the website is a nice and readable exposure to IPC and will, in general, serve your day-to-day needs.


The `execvp` and `execlp` functions are part of the `exec` family of system calls in Linux, used to execute new programs from within a process. Both replace the current process image with a new process image, but they differ in how they handle arguments.

---

### **Key Differences Between `execvp` and `execlp`:**

| Feature             | `execvp`                                          | `execlp`                                             |
|---------------------|---------------------------------------------------|-----------------------------------------------------|
| **Argument Type**   | Takes arguments as an array (vector) of pointers. | Takes arguments as a list of strings (varargs).    |
| **Use Case**        | Suitable when arguments are already in an array.  | Suitable when you know arguments at compile time.  |
| **Prototype**       | ```int execvp(const char *file, char *const argv[]);``` | ```int execlp(const char *file, const char *arg, ...);``` |
| **Ease of Use**     | Preferred in dynamic scenarios where argument list is constructed at runtime. | Preferred for fixed or small numbers of arguments.  |
| **Code Example**    | See examples below.                               | See examples below.                                 |

---

### **Details:**

1. **`execvp`:**
   - **`v`** stands for "vector," meaning it accepts a vector (array) of arguments.
   - **Usage:** If you have arguments stored in an array, use `execvp`.
   - **Example:**
     ```c
     #include <unistd.h>
     int main() {
         char *args[] = {"ls", "-l", "-h", NULL};  // Array of arguments
         execvp("ls", args);                      // Replace process with "ls -l -h"
         return 0;
     }
     ```

2. **`execlp`:**
   - **`l`** stands for "list," meaning it accepts a variable-length list of arguments.
   - **Usage:** If you have a fixed number of arguments known at compile time, use `execlp`.
   - **Example:**
     ```c
     #include <unistd.h>
     int main() {
         execlp("ls", "ls", "-l", "-h", NULL);    // List of arguments
         return 0;
     }
     ```

---

### **Shared Features:**
- **`p`** in both names stands for **"PATH."**
  - Both functions search for the program in the `PATH` environment variable.
  - If the program is not found, they return `-1` and set `errno`.

- **Behavior:**
  - Both replace the current process entirely with the new program.
  - They do not return unless there's an error.

---

### **When to Use:**
- Use **`execvp`** if you work with dynamically constructed arguments.
- Use **`execlp`** if the number and content of arguments are predefined and static.