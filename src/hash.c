#include "hash.h"

#define SHA256_VALUE_LEN 32

void init_hash()
{
    const char *path = "/home/pi/gateway/bin/gateway";
    unsigned char hash[32]; // SHA-256 해시 크기 = 32바이트

    memset(hash, 0, sizeof(hash));

    calculate_sha256(path, hash);

    printf("해시코드 확인: ");
    for (int i = 0; i < 32; i++)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");

    char hashCode[SHA256_VALUE_LEN * 2 + 1];
    for (int i = 0; i < SHA256_VALUE_LEN; i++)
    {
        snprintf(hashCode + (i * 2), sizeof(hashCode), "%02x", hash[i]);
    }
    hashCode[SHA256_VALUE_LEN * 2 + 1] = '\0';

    char query_str[BUFFER_SIZE];
    snprintf(query_str, sizeof(query_str), "update t_info set hash = \'%s\';", hashCode);

    MYSQL *conn = get_conn();

    execute_query(conn, query_str);

    release_conn(conn);
}

void calculate_sha256(const char *file_path, unsigned char *output_hash)
{
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        printf("failed file open!!\n");
        exit(1);
    }

    // EVP SHA256 컨텍스트 생성
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        printf("EVP_MD_CTX_new() failed!!\n");
        fclose(file);
        exit(1);
    }

    // SHA-256 해시 계산 초기화
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1)
    {
        printf("EVP_DigestInit_ex() failed!!\n");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(1);
    }

    // 파일을 읽어서 SHA-256 해시 업데이트
    unsigned char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1)
        {
            printf("EVP_DigestUpdate() failed!!\n");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            exit(1);
        }
    }

    // 최종 해시 계산
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, output_hash, &hash_len) != 1)
    {
        printf("EVP_DigestFinal_ex() failed!!\n");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(1);
    }

    // 컨텍스트 해제
    EVP_MD_CTX_free(mdctx);

    // 파일 닫기
    fclose(file);
}
