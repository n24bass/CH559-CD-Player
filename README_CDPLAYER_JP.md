# CH559 + YAMAHA CRW-F1UX CD-DA制御プロトタイプ

## 目的
CH559のUSBホスト機能からUSB Mass Storage Bulk-Only Transport (BOT) を使い、SCSI/MMCコマンドでYAMAHA CRW-70またはCRW-F1UXにCD-DA再生を指示する試作です。音声データはUSB転送せず、ドライブのアナログLINE OUTを使用します。

## ビルド
1. ZIPを展開します。
2. Windows環境で `compile.bat` または Linux 環境で `make` を実行します（sdcc が必要です）。
3. 成功すると `CH559_CDPlayer.hex` と `CH559_CDPlayer.bin` が生成されます。
4. compile.batは自動書き込みを行いません。書き込み前に対象CH559基板とUSB電源回路を確認してください。

## UART
UART0 115200 bps、元プロジェクトと同じ alternate pin設定（P0.2 TX / P0.3 RX）です。
次のコマンドでCD Playerを操作します。

- `i`: INQUIRY
- `t`: TEST UNIT READY
- `s`: REQUEST SENSE
- `r`: READ TOC
- `p`: カレントトラック再生（READ TOC後）
- `f`: カレントトラック+1再生（READ TOC後）
- `b`: カレントトラック-1再生（READ TOC後）
- `a`: PAUSE
- `c`: RESUME
- `x`: STOP
- `?`: メニュー

## 接続・電源
- CRW-F1UXは純正ACアダプタで給電します。CH559基板のUSB 5Vからドライブを給電しないでください。
- CH559ホスト側USBコネクタのVBUS供給能力を確認してください。VBUSスイッチ／過電流保護を推奨します。
- アナログ音声はCRW-F1UX背面LINE OUT RCA L/Rからアンプへ接続します。

## 重要な制約・未検証点
このソースは `CH559sdccUSBHost` のホスト転送実装にMSC/BOTを追加した「実機評価用の初期ポート」です。USB機器はCRW-70(CRW-F1UX) 1台をRoot Hubへ直結する前提で、USBハブや複数デバイスは対象外です。

特に以下は実機で確認・調整が必要です。

1. 元USBHost.cはHID向けサンプルで、USB Enumerationの処理は既存のままです。MSC interface descriptorとBulk IN/OUT endpointを追加認識するよう拡張しています。
2. USB HostコントローラのDMAバッファは元実装の固定64-byte Rx/Txバッファを使います。Bulk endpoint max packet sizeが64以下であることをチェックします（Full-Speed Bulkは最大64 bytes）。
3. BOTのCSW residue、STALL回復、短いDATA IN、NAK再試行は最小限の実装です。実機ログに応じた調整が必要です。
4. `GET MAX LUN` がSTALLするデバイスは通常LUN 0のみとして扱います。
5. READ TOCはFormat 0 / MSFで最大256 bytes取得します。現状のトラック再生処理は最初のトラックのみを対象にし、次トラック開始位置またはTOCのLead-outを終端に使います。
6. `PLAY AUDIO MSF` は10-byte CDBとして実装しています。
7. CRW-F1UXのUSBブリッジがPLAY AUDIO MSF / PAUSE-RESUMEなどのMMCコマンドを通すかどうかは未確認です。最初はINQUIRYとREAD TOCを確認し、その後PLAYを試してください。
8. BOT Reset RecoveryやClear Featureが失敗する場合、USBHost_Controlの制御転送タイミング・DATA toggle処理の調整が必要です。

## 元コードについて
ベースは `atc1441/CH559sdccUSBHost`。
