void CPage3::OnButtonEncrypt()
{
    // TODO: Add your control notification handler code here
    unsigned char key_hex[256] = {0};
    unsigned char data_hex[256] = {0};
    unsigned char initval_hex[256] = {0};
    unsigned char temp[256] = {0};
    int i = 0;
    int keylen = 0;
    int datalen = 0;
    int InitialLen = 0;
    AES_KEY key;

    UpdateData(TRUE);

    m_key.Remove(' ');
    m_data.Remove(' ');
    m_initval.Remove(' ');

    keylen = m_key.GetLength() / 2;
    datalen = m_data.GetLength() / 2;
    InitialLen = m_initval.GetLength() / 2;

    if (datalen % 16 != 0)
    {
        AfxMessageBox("输入数据长度不是16的整数倍，请重新输入!");
        return;
    }

    StrToHex(m_key, key_hex, keylen);
    StrToHex(m_data, data_hex, datalen);
    StrToHex(m_initval, initval_hex, InitialLen);

    if (keylen == 16)
    {
        // 设置加密密钥
        AES_set_encrypt_key(key_hex, 128, &key);
    }
    else if (keylen == 24)
    {
        // 设置加密密钥
        AES_set_encrypt_key(key_hex, 192, &key);
    }
    else if (keylen == 32)
    {
        // 设置加密密钥
        AES_set_encrypt_key(key_hex, 256, &key);
    }
    else
    {
        AfxMessageBox("输入密钥长度不是16/24/32字节，请重新输入!");
        return;
    }

    // ECB模式
    if (((CButton *)GetDlgItem(IDC_RADIO1))->GetCheck())
    {
        for (i = 0; i < datalen / 16; i++)
        {
            AES_ecb_encrypt(data_hex + i * 16, temp + i * 16, &key, AES_ENCRYPT);
        }
    }
    // CBC模式
    else if (((CButton *)GetDlgItem(IDC_RADIO2))->GetCheck())
    {
        memcpy(data_hex + datalen, "\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);

        datalen = datalen + 16;

        for (i = 0; i < datalen / 16; i++)
        {
            AES_cbc_encrypt(data_hex + i * 16, temp + i * 16, 16, &key, initval_hex, AES_ENCRYPT);
        }
    }

    HexToStr(temp, datalen, m_result);

    UpdateData(FALSE);
}
