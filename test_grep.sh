echo "I2C_LL_Set(pConfig->freq);" | grep -q 'Config->.*[*/<>]' && echo "MATCH 1"
echo "uint32_t x = pConfig->pclk / 1000;" | grep -q 'Config->.*[*/<>]\|[*/<>].*Config->' && echo "MATCH 2"
echo "val = (pConfig->speed << 2);" | grep -q 'Config->.*[*/<>]\|[*/<>].*Config->' && echo "MATCH 3"
