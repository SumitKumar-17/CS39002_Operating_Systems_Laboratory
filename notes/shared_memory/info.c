/* One shmid data structure for each shared memory segment in the system. */
struct shmid_ds {
  struct ipc_perm shm_perm; /* operation perms */
  int shm_segsz;            /* size of segment (bytes) */
  time_t shm_atime;         /* last attach time */
  time_t shm_dtime;         /* last detach time */
  time_t shm_ctime;         /* last change time */
  unsigned short shm_cpid;  /* pid of creator */
  unsigned short shm_lpid;  /* pid of last operator */
  short shm_nattch;         /* no. of current attaches */
  /* the following are private */
  unsigned short shm_npages;       /* size of segment (pages) */
  unsigned long *shm_pages;        /* array of ptrs to frames -> SHMMAX */
  struct vm_area_struct *attaches; /* descriptors for attaches */
};

struct ipc_perm {
  key_t key;
  ushort uid; /* user euid and egid */
  ushort gid;
  ushort cuid; /* creator euid and egid */
  ushort cgid;
  ushort mode; /* access modes see mode flags below
                */
}