#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

/* JOLIET: The specification only allows filenames to be up to 64
Unicode characters in length. However, the documentation for mkisofs
states filenames up to 103 characters in length do not appear to cause
problems.[34] Microsoft has documented it "can use up to 110
characters." */

const size_t DVD_SIZE = 4700000000;

struct allfile
{
  struct dirent from_opendir;
  struct stat from_stat;
};

struct bin
{
  size_t free_space;
  struct allfile * files[10];
};

size_t sum_size(struct allfile * file_list, unsigned int number_of_files)
{
  size_t sum = 0;
  for (int i = 0; i < number_of_files; i++)
  {
    sum += file_list[i].from_stat.st_size;
  }
  return sum;
}

int count_files(DIR *dir_handle)
{
  unsigned int number_of_files = 0;
  struct dirent *entry;
  while ( (entry = readdir(dir_handle)) != NULL )
  {
    if (entry->d_type == DT_REG)
    {
      number_of_files++;
    }
  }
  rewinddir(dir_handle);
  return number_of_files;
}

struct allfile * create_filelist(char * path, unsigned int * number_of_files)
{
  int n = 0;
  DIR *dir_handle = opendir(path);
  struct dirent *entry;
  struct stat buf;
  chdir(path);
  *number_of_files = count_files(dir_handle);
  struct allfile * file_list = malloc(*number_of_files * sizeof(struct allfile));

  while ( (entry = readdir(dir_handle)) != NULL )
  {
    if (entry->d_type == DT_REG)
    {
      stat(entry->d_name, &buf);
      file_list[n].from_opendir =  *entry;
      file_list[n].from_stat =  buf;
      n++;
    }
  }
  closedir(dir_handle);
  return file_list;
}

void helper_print(struct allfile *file_list, unsigned int count)
{
  for (int i = 0 ; i < count; i++)
  {
    printf("%s: %ld b\n", file_list[i].from_opendir.d_name, file_list[i].from_stat.st_size);
  }


}

unsigned int count_max_disks(struct allfile * file_list, unsigned int number_of_files)
{
  size_t total_size = sum_size(file_list, number_of_files);
  float split = total_size / (float) DVD_SIZE;
  unsigned int m = ceil(split);
  unsigned int max = (4 * m + 1)/3;
  return max;
}

struct bin * init_bins(struct allfile *file_list, unsigned int number_of_files, unsigned int * max_disks)
{
  *max_disks = count_max_disks(file_list, number_of_files);
  struct bin * bins = malloc(*max_disks * sizeof(struct bin));
  for (int b = 0; b < *max_disks; b++)
  {
    bins[b].free_space = DVD_SIZE;
    for (int i = 0; i < 10; i++)
    {
      bins[b].files[i] = NULL;
    }
  }
  printf("Celkem %ld, maximálně %u disků\n", sum_size(file_list, number_of_files), *max_disks);
  return bins;
}

int md_comparator(const void *v1, const void *v2)
{
    const struct allfile *p1 = (struct allfile *)v1;
    const struct allfile *p2 = (struct allfile *)v2;
    if (p1->from_stat.st_size < p2->from_stat.st_size)
        return +1;
    else if (p1->from_stat.st_size > p2->from_stat.st_size)
        return -1;
    else
        return 0;
}

unsigned int first_fit(struct allfile *file_list, unsigned int number_of_files, struct bin *bins, unsigned int number_of_bins)
{
  for (int f = 0; f < number_of_files; f++)
  {
    for (int b = 0; b < number_of_bins; b++)
    {
      if (bins[b].free_space > file_list[f].from_stat.st_size)
      {
        bins[b].free_space = bins[b].free_space - file_list[f].from_stat.st_size;
        for (int n = 0; n < 10; n++)
        {
          if (!bins[b].files[n])
          {
            bins[b].files[n] = &file_list[f];
            /*printf("%s TO BIN %d\n", file_list[f].from_opendir.d_name, b);*/
            break;
          }
        }
        break;
      }
    }
  }

  return 0;
}

void print_bins(struct bin *bins, unsigned int number_of_bins)
{
  for (int b = 0; b < number_of_bins; b++)
  {
    if (bins[b].free_space == DVD_SIZE)
    {
      continue;
    }
    for (int n = 0; n < 10; n++)
    {
      if (bins[b].files[n])
      {
        printf("%s\n", bins[b].files[n]->from_opendir.d_name);
      }
    }
    printf("Disk: %d, volné místo %ld MB \n\n", b, bins[b].free_space >> 20);
  }
}

int main(int argc, char *argv[])
{
  unsigned int count;
  unsigned int max_disks = 0;
  struct allfile *file_list = create_filelist(argv[1], &count);
  qsort(file_list, count, sizeof(struct allfile), md_comparator);
  /*helper_print(file_list, count);*/
  struct bin * bins = init_bins(file_list, count, &max_disks);

  first_fit(file_list, count, bins, max_disks);
  print_bins(bins, max_disks);

  free(file_list);
  free(bins);
}

