//
// Created by 二阶堂天宇 on 2021/9/23.
//
#include "math.h"
#include <fcntl.h>
#include <iostream>
#include <liburing.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

char *test_buf = (char *)"abc";
struct io_uring *ioring;

struct file_page {
  file_page(int pages) { iov = (iovec *)calloc(pages, sizeof(struct iovec)); }

  struct iovec *iov;
};

// 待改写函数 1
bool PosixWrite(int fd, char *buf, size_t nbyte) {
  const size_t kLimit1Gb = 1UL << 30;

  const char *src = buf;
  size_t left = nbyte;

  while (left != 0) {
    size_t bytes_to_write = std::min(left, kLimit1Gb);

    ssize_t done = write(fd, src, bytes_to_write);
    if (done < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    left -= done;
    src += done;
  }
  return true;
}

// 以 n 模式改写函数 1
bool NPosixWrite(int fd, const char *buf, size_t nbyte) {
  static const int PageSize = 4096;
  //
  int pages = (int)std::ceil((float)nbyte / PageSize);
  int last_page_size = nbyte % PageSize;
  int page_size = PageSize;
  char *no_const_buf = const_cast<char *>(buf);
  struct iovec iov[pages];
  for (int i = 0; i < pages; i++) {
    iov[i].iov_base = no_const_buf + i * page_size;
    if (i == pages - 1 && last_page_size != 0)
      page_size = last_page_size;
    iov[i].iov_len = page_size;
  }
  writev(fd, iov, pages);
  return true;
}

// 以 i 模式改写函数 1
bool IPosixWrite(int fd, const char *buf, size_t nbyte) {
  static const int PageSize = 4096;
  //
  int pages = (int)std::ceil((float)nbyte / PageSize);
  int last_page_size = nbyte % PageSize;
  int page_size = PageSize;
  file_page *data = new file_page(pages);
  char *no_const_buf = const_cast<char *>(buf);
  for (int i = 0; i < pages; i++) {
    data->iov[i].iov_base = no_const_buf + i * page_size;
    if (i == pages - 1 && last_page_size != 0)
      page_size = last_page_size;
    data->iov[i].iov_len = page_size;
  }

  auto sqe = io_uring_get_sqe(ioring);
  io_uring_prep_writev(sqe, fd, data->iov, pages, 0);
  io_uring_sqe_set_data(sqe, data);
  io_uring_submit(ioring);
  return true;
}

// 待改写函数 2
bool PosixPositionedWrite(int fd, const char *buf, size_t nbyte, off_t offset) {
  const size_t kLimit1Gb = 1UL << 30;

  const char *src = buf;
  size_t left = nbyte;

  while (left != 0) {
    size_t bytes_to_write = std::min(left, kLimit1Gb);

    ssize_t done = pwrite(fd, src, bytes_to_write, offset);
    if (done < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    left -= done;
    offset += done;
    src += done;
  }

  return true;
}

// 以 n 模式改写函数 2, current can not run in mac 10
bool NPosixPositionedWrite(int fd, const char *buf, size_t nbyte,
                           off_t offset) {
  static const int PageSize = 4096;
  //
  int pages = (int)std::ceil((float)nbyte / PageSize);
  int last_page_size = nbyte % PageSize;
  int page_size = PageSize;
  char *no_const_buf = const_cast<char *>(buf);
  struct iovec iov[pages];
  for (int i = 0; i < pages; i++) {
    iov[i].iov_base = no_const_buf + i * page_size;
    if (i == pages - 1 && last_page_size != 0)
      page_size = last_page_size;
    iov[i].iov_len = page_size;
  }
  pwritev(fd, iov, pages, offset);
  return true;
}

int get_completion_and_print() {
  struct io_uring_cqe *cqe;
  auto ret = io_uring_wait_cqe(ioring, &cqe);
  if (ret == 0 && cqe->res >= 0) {
    struct file_page *fi = (file_page *)io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(ioring, cqe);
    free(fi->iov[0].iov_base);
  }
  return 0;
}

// 参数 1: 文件路径 参数 2: 函数是否是 p 参数 3：测试模式 i, n, o
int main(int argc, char *argv[]) {
  if (argc < 4) {
    argv[1] =
        (char *)"/Users/tianyugou/git_project/testingground/src/io_uring/data.txt";
    argv[2] = (char *)"p";
    argv[3] = (char *)"n";
  }
  int outfd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (strcmp(argv[2], "np") == 0 && strcmp(argv[3], "o") == 0) {
    PosixWrite(outfd, test_buf, sizeof(test_buf));
  }
  if (strcmp(argv[2], "np") == 0 && strcmp(argv[3], "n") == 0) {
    NPosixWrite(outfd, test_buf, sizeof(test_buf));
  }
  if (strcmp(argv[2], "np") == 0 && strcmp(argv[3], "i") == 0) {
    // TODO
    io_uring_queue_init(1, ioring, 0);
    IPosixWrite(outfd, test_buf, sizeof(test_buf));
    get_completion_and_print();
  }
  if (strcmp(argv[2], "p") == 0 && strcmp(argv[3], "o") == 0) {
    PosixPositionedWrite(outfd, test_buf, sizeof(test_buf), 1);
  }
  if (strcmp(argv[2], "p") == 0 && strcmp(argv[3], "n") == 0) {
    NPosixPositionedWrite(outfd, test_buf, sizeof(test_buf), 1);
  }
  if (strcmp(argv[2], "p") == 0 && strcmp(argv[3], "i") == 0) {
    // TODO
  }
  return 0;
}