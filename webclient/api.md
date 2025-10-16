# 系统信息接口文档

## 1. CPU 信息
- **接口路径**: `/api/cpu/info`
- **描述**: 获取 CPU 的详细信息
- **响应示例**:
  ```json
  {
      "architecture": "x64",
      "baseFrequency": 3187,
      "logicalCores": 16,
      "maxFrequency": 3435973836,
      "name": "13th Gen Intel(R) Core(TM) i5-13500H",
      "packages": 1,
      "physicalCores": 12,
      "vendor": ""
  }
  ```

## 2. 系统信息
- **接口路径**: `/api/system/info`
- **描述**: 获取系统整体信息，包括 CPU 使用情况
- **响应示例**:
  ```json
  {
      "cpu": {
          "currentUsage": 8.897485493230175,
          "info": {
              "cores": 12,
              "name": "13th Gen Intel(R) Core(TM) i5-13500H",
              "threads": 16
          }
      },
      "timestamp": 1012987656
  }
  ```

## 3. CPU 使用率
- **接口路径**: `/api/cpu/usage`
- **描述**: 获取当前 CPU 使用率
- **响应示例**:
  ```json
  {
      "timestamp": 1012949593,
      "unit": "percent",
      "usage": 12.366114897760468
  }
  ```

## 4. CPU 使用率流
- **接口路径**: `/api/cpu/stream`
- **描述**: 获取实时 CPU 使用率流
- **响应示例**:
  ```json
  data: {
      "timestamp": 1013046250,
      "usage": 7.783251231527093
  }
  ```

## 5. 进程列表
- **接口路径**: `/api/processes`
- **描述**: 获取当前运行的进程列表
- **响应示例**:
  ```json
  {
      "processes": [
          {
              "commandLine": "",
              "cpuUsage": -9.255963134931783e+61,
              "createTime": -3689348814741910324,
              "fullPath": "",
              "memoryUsage": 14757395258967641292,
              "name": "[System Process]",
              "pagefileUsage": 14757395258967641292,
              "parentPid": 0,
              "pid": 0,
              "priority": 0,
              "state": "Running",
              "threadCount": 48,
              "username": "",
              "workingSetSize": 14757395258967641292
          },
          {
              "commandLine": "",
              "cpuUsage": -9.255963134931783e+61,
              "createTime": -3689348814741910324,
              "fullPath": "",
              "memoryUsage": 14757395258967641292,
              "name": "System",
              "pagefileUsage": 14757395258967641292,
              "parentPid": 0,
              "pid": 4,
              "priority": 8,
              "state": "Running",
              "threadCount": 442,
              "username": "",
              "workingSetSize": 14757395258967641292
          },
          {
              "commandLine": "",
              "cpuUsage": 0.0,
              "createTime": 134047266154156169,
              "fullPath": "",
              "memoryUsage": 119578624,
              "name": "svchost.exe",
              "pagefileUsage": 64622592,
              "parentPid": 1784,
              "pid": 44364,
              "priority": 8,
              "state": "Running",
              "threadCount": 6,
              "username": "",
              "workingSetSize": 119578624
          }
      ],
      "timestamp": 1013085171,
      "totalProcesses": 354,
      "totalThreads": 6039
  }
  ```

## 6. 指定进程信息
- **接口路径**: `/api/process/{pid}`
- **描述**: 获取指定 PID 的进程详细信息
- **响应示例**:
  ```json
  {
      "commandLine": "",
      "cpuUsage": 0.0,
      "createTime": 134047266154156169,
      "fullPath": "",
      "memoryUsage": 127303680,
      "name": "svchost.exe",
      "pagefileUsage": 71913472,
      "parentPid": 1784,
      "pid": 44364,
      "priority": 8,
      "state": "Running",
      "threadCount": 6,
      "username": "",
      "workingSetSize": 127303680
  }
  ```

## 7. 查找进程
- **接口路径**: `/api/process/find?name={name}`
- **描述**: 根据名称查找进程
- **响应示例**:
  ```json
  [
      {
          "cpuUsage": -9.255963134931783e+61,
          "memoryUsage": 14757395258967641292,
          "name": "svchost.exe",
          "pid": 1960,
          "threadCount": 12,
          "username": ""
      },
      {
          "cpuUsage": -9.255963134931783e+61,
          "memoryUsage": 14757395258967641292,
          "name": "svchost.exe",
          "pid": 1056,
          "threadCount": 11,
          "username": ""
      },
      {
          "cpuUsage": 0.0,
          "memoryUsage": 127385600,
          "name": "svchost.exe",
          "pid": 44364,
          "threadCount": 4,
          "username": ""
      }
  ]
  ```

## 8. 终止进程
- **接口路径**: `/api/process/{pid}/terminate`
- **描述**: 终止指定 PID 的进程
- **响应示例**:
  ```json
  {
      "pid": 37232,
      "success": true
  }
  ```

## 9. 磁盘信息
- **接口路径**: `/api/disk/info`
- **描述**: 获取磁盘驱动器和分区的详细信息
- **响应示例**:
  ```json
  {
      "drives": [
          {
              "bytesPerSector": 512,
              "deviceId": "C:\\",
              "interfaceType": "Fixed",
              "mediaType": "Unknown",
              "model": "Windows-SSD",
              "serialNumber": "",
              "status": "Healthy",
              "totalSize": 214749409280
          },
          {
              "bytesPerSector": 512,
              "deviceId": "D:\\",
              "interfaceType": "Fixed",
              "mediaType": "Unknown",
              "model": "Data",
              "serialNumber": "",
              "status": "Healthy",
              "totalSize": 294972813312
          },
          {
              "bytesPerSector": 512,
              "deviceId": "\\\\.\\PHYSICALDRIVE0",
              "interfaceType": "SCSI",
              "mediaType": "Fixed hard disk media",
              "model": "Micron MTFDKBA512TFH",
              "serialNumber": "00A0_7501_3CD7_57CF.",
              "status": "Healthy",
              "totalSize": 2298335765560
          }
      ],
      "partitions": [
          {
              "driveLetter": "C:",
              "fileSystem": "NTFS",
              "freeSpace": 29133791232,
              "label": "Windows-SSD",
              "serialNumber": 2764861816,
              "totalSize": 214749409280,
              "usagePercentage": 86.4335872542429,
              "usedSpace": 185615618048
          },
          {
              "driveLetter": "D:",
              "fileSystem": "NTFS",
              "freeSpace": 85361258496,
              "label": "Data",
              "serialNumber": 513519313,
              "totalSize": 294972813312,
              "usagePercentage": 71.06131323170068,
              "usedSpace": 209611554816
          }
      ],
      "timestamp": 1013836593
  }
  ```

## 10. 磁盘性能
- **接口路径**: `/api/disk/performance`
- **描述**: 获取磁盘性能数据
- **响应示例**:
  ```json
  {
      "performance": [
          {
              "driveLetter": "C:",
              "queueLength": 0.0,
              "readBytesPerSec": 0,
              "readCountPerSec": 0,
              "readSpeed": 0.0,
              "responseTime": 0,
              "usagePercentage": 0.0,
              "writeBytesPerSec": 0,
              "writeCountPerSec": 0,
              "writeSpeed": 0.0
          },
          {
              "driveLetter": "D:",
              "queueLength": 0.0,
              "readBytesPerSec": 0,
              "readCountPerSec": 0,
              "readSpeed": 0.0,
              "responseTime": 0,
              "usagePercentage": 0.0,
              "writeBytesPerSec": 0,
              "writeCountPerSec": 0,
              "writeSpeed": 0.0
          }
      ],
      "timestamp": 1013904296
  }
  ```


## 11. 内存使用情况

/api/memory/usage
```json
{"availablePhysical":17262604288,"timestamp":1382173734,"totalPhysical":34071240704,"unit":"bytes","usedPercent":49.333796095152614,"usedPhysical":16808636416}
```


## 12. cpu使用历史
/api/cpu/history
```json
[{"timestamp":1382039671,"value":0.0},{"timestamp":1382040687,"value":19.43127962085308},{"timestamp":1382041687,"value":19.781312127236582},{"timestamp":1382042703,"value":18.22759315206445},{"timestamp":1382043718,"value":37.96548592188919},{"timestamp":1382044734,"value":34.07122232916266},{"timestamp":1382045734,"value":28.125},{"timestamp":1382046750,"value":22.27579556412729},{"timestamp":1382047750,"value":15.794573643410853},{"timestamp":1382048750,"value":22.691552062868368},{"timestamp":1382049765,"value":35.46845124282983},{"timestamp":1382050781,"value":22.00772200772201},{"timestamp":1382051781,"value":33.75486381322957},{"timestamp":1382052781,"value":36.18756371049949},{"timestamp":1382053781,"value":19.32139491046183},{"timestamp":1382054796,"value":30.11472275334608},{"timestamp":1382055796,"value":23.33984375},{"timestamp":1382056812,"value":39.42307692307692},{"timestamp":1382057812,"value":26.3671875},{"timestamp":1382058828,"value":37.40384615384615},{"timestamp":1382059828,"value":30.46875},{"timestamp":1382060843,"value":31.74757281553398},{"timestamp":1382061843,"value":34.91295938104449},{"timestamp":1382062859,"value":28.330019880715707},{"timestamp":1382063859,"value":22.30843840931135},{"timestamp":1382064875,"value":23.617619493908155},{"timestamp":1382065890,"value":21.182266009852217},{"timestamp":1382066890,"value":20.177165354330707},{"timestamp":1382067906,"value":19.1588785046729},{"timestamp":1382068906,"value":13.437195715676728},{"timestamp":1382069906,"value":20.974155069582505},{"timestamp":1382070921,"value":17.267552182163186},{"timestamp":1382071937,"value":22.42540904716073},{"timestamp":1382072953,"value":16.30859375},{"timestamp":1382073953,"value":25.04835589941973},{"timestamp":1382074953,"value":20.382775119617225},{"timestamp":1382075968,"value":29.852216748768473},{"timestamp":1382076968,"value":42.1256038647343},{"timestamp":1382077984,"value":22.840690978886755},{"timestamp":1382078984,"value":22.740814299900695},{"timestamp":1382080000,"value":23.32389046270066},{"timestamp":1382081015,"value":21.16504854368932},{"timestamp":1382082015,"value":12.657004830917874},{"timestamp":1382083031,"value":24.784688995215312},{"timestamp":1382084046,"value":17.369970559371932},{"timestamp":1382085062,"value":11.291866028708133},{"timestamp":1382086078,"value":22.33940556088207},{"timestamp":1382087078,"value":48.9815712900097},{"timestamp":1382088078,"value":25.0},{"timestamp":1382089093,"value":33.07543520309478},{"timestamp":1382090093,"value":30.029440628066734},{"timestamp":1382091109,"value":30.44719314938154},{"timestamp":1382092109,"value":30.498533724340177},{"timestamp":1382093125,"value":41.59462055715658},{"timestamp":1382094125,"value":43.45703125},{"timestamp":1382095125,"value":69.53125},{"timestamp":1382096140,"value":58.07692307692308},{"timestamp":1382097140,"value":44.84126984126984},{"timestamp":1382098156,"value":36.26893939393939},{"timestamp":1382099171,"value":37.01923076923077},{"timestamp":1382100171,"value":49.21259842519685},{"timestamp":1382101187,"value":30.813953488372093},{"timestamp":1382102187,"value":29.406130268199234},{"timestamp":1382103203,"value":18.511263467189032},{"timestamp":1382104218,"value":25.166191832858498},{"timestamp":1382105218,"value":16.03128054740958},{"timestamp":1382106218,"value":23.758519961051608},{"timestamp":1382107234,"value":42.59615384615385},{"timestamp":1382108250,"value":31.528046421663444},{"timestamp":1382109250,"value":19.82421875},{"timestamp":1382110250,"value":21.994134897360702},{"timestamp":1382111265,"value":31.70028818443804},{"timestamp":1382112265,"value":9.24124513618677},{"timestamp":1382113281,"value":10.65891472868217},{"timestamp":1382114281,"value":18.333333333333332},{"timestamp":1382115281,"value":13.575525812619503},{"timestamp":1382116296,"value":11.271676300578035},{"timestamp":1382117312,"value":23.307543520309476},{"timestamp":1382118328,"value":11.218568665377177},{"timestamp":1382119328,"value":25.867195242814667},{"timestamp":1382120343,"value":11.18546845124283},{"timestamp":1382121343,"value":12.60827718960539},{"timestamp":1382122343,"value":29.538763493621197},{"timestamp":1382123343,"value":19.880715705765407},{"timestamp":1382124359,"value":13.257575757575758},{"timestamp":1382125359,"value":10.063559322033898},{"timestamp":1382126375,"value":14.774774774774775},{"timestamp":1382127390,"value":11.164122137404581},{"timestamp":1382128406,"value":21.5},{"timestamp":1382129406,"value":20.26266416510319},{"timestamp":1382130421,"value":14.708691499522445},{"timestamp":1382131437,"value":10.192307692307692},{"timestamp":1382132437,"value":12.961116650049851},{"timestamp":1382133437,"value":31.874405328258803},{"timestamp":1382134453,"value":13.352826510721247},{"timestamp":1382135453,"value":16.17497456765005},{"timestamp":1382136468,"value":14.232902033271719},{"timestamp":1382137468,"value":17.129186602870814},{"timestamp":1382138484,"value":34.649555774925965},{"timestamp":1382139500,"value":27.497621313035204},{"timestamp":1382140500,"value":17.140058765915768},{"timestamp":1382141500,"value":18.561710398445094},{"timestamp":1382142515,"value":17.91791791791792},{"timestamp":1382143515,"value":14.741784037558686},{"timestamp":1382144531,"value":24.95164410058027},{"timestamp":1382145531,"value":15.415415415415415},{"timestamp":1382146531,"value":37.0722433460076},{"timestamp":1382147531,"value":37.682570593963},{"timestamp":1382148531,"value":33.84615384615385},{"timestamp":1382149546,"value":24.72906403940887},{"timestamp":1382150562,"value":15.452755905511811},{"timestamp":1382151562,"value":15.421002838221382},{"timestamp":1382152562,"value":23.892893923789906},{"timestamp":1382153578,"value":20.768526989935957},{"timestamp":1382154593,"value":30.28846153846154},{"timestamp":1382155593,"value":17.716535433070867},{"timestamp":1382156609,"value":37.34359961501444},{"timestamp":1382157625,"value":32.98379408960915},{"timestamp":1382158640,"value":38.379942140790746},{"timestamp":1382159640,"value":49.608610567514674},{"timestamp":1382160640,"value":41.15347018572825},{"timestamp":1382161656,"value":25.52581261950287},{"timestamp":1382162671,"value":25.442043222003928},{"timestamp":1382163671,"value":33.23671497584541},{"timestamp":1382164671,"value":23.67149758454106},{"timestamp":1382165671,"value":29.8828125},{"timestamp":1382166671,"value":30.847803881511748},{"timestamp":1382167687,"value":27.557603686635943},{"timestamp":1382168687,"value":36.42578125},{"timestamp":1382169703,"value":27.965284474445514},{"timestamp":1382170718,"value":32.208293153326906},{"timestamp":1382171718,"value":21.223709369024856},{"timestamp":1382172734,"value":21.341463414634145},{"timestamp":1382173734,"value":14.661654135338345},{"timestamp":1382174734,"value":19.39275220372184},{"timestamp":1382175734,"value":23.099415204678362},{"timestamp":1382176750,"value":34.5821325648415},{"timestamp":1382177750,"value":18.75},{"timestamp":1382178750,"value":25.172074729596854},{"timestamp":1382179781,"value":23.033175355450236},{"timestamp":1382180781,"value":26.25968992248062},{"timestamp":1382181781,"value":25.606207565470417},{"timestamp":1382182796,"value":21.047331319234644},{"timestamp":1382183796,"value":13.909774436090226},{"timestamp":1382184796,"value":17.5},{"timestamp":1382185812,"value":26.785714285714285},{"timestamp":1382186812,"value":19.065776930409914},{"timestamp":1382187828,"value":28.322017458777886},{"timestamp":1382188828,"value":23.465346534653467},{"timestamp":1382189843,"value":25.523809523809526},{"timestamp":1382190843,"value":25.90068159688413},{"timestamp":1382191843,"value":27.48768472906404},{"timestamp":1382192843,"value":29.242569511025888},{"timestamp":1382193859,"value":26.341463414634145},{"timestamp":1382194875,"value":23.04015296367113},{"timestamp":1382195890,"value":28.309178743961354},{"timestamp":1382196890,"value":22.06025267249757},{"timestamp":1382197906,"value":22.307692307692307},{"timestamp":1382198921,"value":24.563106796116504},{"timestamp":1382199937,"value":27.904761904761905},{"timestamp":1382200953,"value":33.84615384615385},{"timestamp":1382201953,"value":33.85826771653543},{"timestamp":1382202968,"value":27.958015267175572},{"timestamp":1382203984,"value":25.02415458937198},{"timestamp":1382205000,"value":32.78846153846154},{"timestamp":1382206000,"value":28.823529411764707},{"timestamp":1382207000,"value":26.153846153846153},{"timestamp":1382208015,"value":27.414634146341463},{"timestamp":1382209015,"value":25.78125},{"timestamp":1382210031,"value":24.522900763358777},{"timestamp":1382211031,"value":28.263002944062805},{"timestamp":1382212031,"value":41.97651663405088},{"timestamp":1382213046,"value":25.02415458937198},{"timestamp":1382214046,"value":20.01943634596696},{"timestamp":1382215062,"value":25.441329179646935},{"timestamp":1382216062,"value":25.412844036697248},{"timestamp":1382217062,"value":26.00767754318618},{"timestamp":1382218078,"value":17.11798839458414},{"timestamp":1382219078,"value":27.63819095477387},{"timestamp":1382220093,"value":22.35734331150608},{"timestamp":1382221109,"value":20.981713185755535},{"timestamp":1382222125,"value":28.351012536162006},{"timestamp":1382223140,"value":14.688715953307392},{"timestamp":1382224140,"value":6.118546845124283},{"timestamp":1382225156,"value":9.23076923076923},{"timestamp":1382226156,"value":31.11545988258317},{"timestamp":1382227171,"value":25.965250965250966},{"timestamp":1382228187,"value":12.138728323699421},{"timestamp":1382229187,"value":18.926829268292682},{"timestamp":1382230187,"value":21.047526673132882},{"timestamp":1382231203,"value":15.673076923076923},{"timestamp":1382232218,"value":21.53846153846154},{"timestamp":1382233218,"value":24.019607843137255},{"timestamp":1382234234,"value":20.881226053639846},{"timestamp":1382235234,"value":9.690721649484535},{"timestamp":1382236250,"value":14.87985212569316},{"timestamp":1382237265,"value":12.91866028708134},{"timestamp":1382238281,"value":8.5824493731919},{"timestamp":1382239281,"value":16.781836130306022},{"timestamp":1382240281,"value":23.9067055393586},{"timestamp":1382241281,"value":16.25},{"timestamp":1382242281,"value":23.949169110459433},{"timestamp":1382243281,"value":24.854368932038835},{"timestamp":1382244296,"value":18.357487922705314},{"timestamp":1382245296,"value":12.730806608357629},{"timestamp":1382246312,"value":20.058997050147493},{"timestamp":1382247328,"value":13.505747126436782},{"timestamp":1382248343,"value":7.495256166982922},{"timestamp":1382249359,"value":21.684414327202322},{"timestamp":1382250375,"value":15.429122468659594},{"timestamp":1382251390,"value":17.714285714285715},{"timestamp":1382252390,"value":17.416829745596868},{"timestamp":1382253406,"value":20.251937984496124},{"timestamp":1382254421,"value":28.790786948176585},{"timestamp":1382255421,"value":17.48046875},{"timestamp":1382256421,"value":26.60287081339713},{"timestamp":1382257437,"value":17.425742574257427},{"timestamp":1382258453,"value":19.73307912297426},{"timestamp":1382259453,"value":22.16796875},{"timestamp":1382260468,"value":25.911330049261085},{"timestamp":1382261484,"value":9.176915799432356},{"timestamp":1382262484,"value":25.76923076923077},{"timestamp":1382263484,"value":16.30859375},{"timestamp":1382264484,"value":37.34115347018573},{"timestamp":1382265500,"value":17.098943323727184}]
```

## 13 内存使用历史
/api/memory/history
```json
[{"timestamp":1382039671,"value":45.36790165726276},{"timestamp":1382040687,"value":45.39182517701602},{"timestamp":1382041687,"value":45.38458801174392},{"timestamp":1382042703,"value":45.367973788478096},{"timestamp":1382043718,"value":47.295115490491064},{"timestamp":1382044734,"value":47.76727440421419},{"timestamp":1382045734,"value":47.77011156535076},{"timestamp":1382046750,"value":47.61131469478758},{"timestamp":1382047750,"value":47.53108073959501},{"timestamp":1382048750,"value":47.62019885614319},{"timestamp":1382049765,"value":47.67926229963451},{"timestamp":1382050765,"value":47.445328746429205},{"timestamp":1382051765,"value":47.492839173597474},{"timestamp":1382052781,"value":47.541924465633926},{"timestamp":1382053781,"value":47.54054195067331},{"timestamp":1382054796,"value":47.48150255092043},{"timestamp":1382055796,"value":47.509946293501436},{"timestamp":1382056796,"value":47.52133100365537},{"timestamp":1382057796,"value":47.297411667512606},{"timestamp":1382058812,"value":47.21459301043715},{"timestamp":1382059812,"value":47.123010410698306},{"timestamp":1382060828,"value":47.260083763576},{"timestamp":1382061828,"value":47.34569149431113},{"timestamp":1382062843,"value":47.313857584609316},{"timestamp":1382063843,"value":47.391386619226765},{"timestamp":1382064859,"value":47.364385500952494},{"timestamp":1382065875,"value":47.30902479318177},{"timestamp":1382066875,"value":47.35515270538943},{"timestamp":1382067890,"value":47.19185965573694},{"timestamp":1382068890,"value":47.179308824268404},{"timestamp":1382069906,"value":47.1758705696707},{"timestamp":1382070921,"value":46.88602330271043},{"timestamp":1382071937,"value":46.88882439823932},{"timestamp":1382072953,"value":46.83104729475483},{"timestamp":1382073953,"value":46.72722643214725},{"timestamp":1382074953,"value":46.74935869338631},{"timestamp":1382075968,"value":46.75059694591626},{"timestamp":1382076968,"value":46.94955888155261},{"timestamp":1382077984,"value":46.84394676043083},{"timestamp":1382078984,"value":47.12642462155757},{"timestamp":1382080000,"value":47.17536565116335},{"timestamp":1382081015,"value":47.13279621224562},{"timestamp":1382082015,"value":47.120738277415214},{"timestamp":1382083031,"value":47.0413819186759},{"timestamp":1382084046,"value":46.98017858246293},{"timestamp":1382085062,"value":46.9981392550817},{"timestamp":1382086078,"value":46.95561790364087},{"timestamp":1382087078,"value":47.01409227554028},{"timestamp":1382088078,"value":47.00443871455442},{"timestamp":1382089093,"value":47.087016934245426},{"timestamp":1382090093,"value":47.174656360879204},{"timestamp":1382091109,"value":47.04734476581038},{"timestamp":1382092109,"value":46.931730449495284},{"timestamp":1382093125,"value":47.01382779441738},{"timestamp":1382094125,"value":47.13267599355339},{"timestamp":1382095125,"value":48.05857631735042},{"timestamp":1382096125,"value":47.81233237006103},{"timestamp":1382097140,"value":47.81187553903056},{"timestamp":1382098140,"value":47.81840341401851},{"timestamp":1382099140,"value":48.16959827962243},{"timestamp":1382100156,"value":49.07708109977021},{"timestamp":1382101171,"value":49.07848765846927},{"timestamp":1382102171,"value":48.95243835966884},{"timestamp":1382103187,"value":48.96477279749137},{"timestamp":1382104187,"value":48.85096176155969},{"timestamp":1382105203,"value":48.82748305096768},{"timestamp":1382106203,"value":48.85284919502766},{"timestamp":1382107218,"value":49.989613104991555},{"timestamp":1382108218,"value":50.11807879950576},{"timestamp":1382109218,"value":50.011432797630825},{"timestamp":1382110234,"value":50.08268641651401},{"timestamp":1382111234,"value":50.11698480940649},{"timestamp":1382112250,"value":50.145224180210704},{"timestamp":1382113265,"value":50.02539018779843},{"timestamp":1382114265,"value":50.12530394290862},{"timestamp":1382115265,"value":50.12204601634926},{"timestamp":1382116281,"value":50.14634221404842},{"timestamp":1382117296,"value":50.09413123601406},{"timestamp":1382118296,"value":50.08148422959174},{"timestamp":1382119312,"value":50.11669628454514},{"timestamp":1382120328,"value":50.09716074705819},{"timestamp":1382121328,"value":50.07065252542205},{"timestamp":1382122343,"value":50.13906898316866},{"timestamp":1382123343,"value":50.23590513975784},{"timestamp":1382124359,"value":50.26710189039085},{"timestamp":1382125359,"value":50.24121880595429},{"timestamp":1382126375,"value":50.19715865525294},{"timestamp":1382127390,"value":50.17770727085055},{"timestamp":1382128406,"value":50.173643879053266},{"timestamp":1382129406,"value":50.14746024788614},{"timestamp":1382130421,"value":50.118559674274664},{"timestamp":1382131437,"value":50.13088209022798},{"timestamp":1382132437,"value":50.1219618632647},{"timestamp":1382133437,"value":50.17381218522238},{"timestamp":1382134437,"value":50.14357718412719},{"timestamp":1382135453,"value":50.03445467719237},{"timestamp":1382136468,"value":50.01835739430312},{"timestamp":1382137468,"value":50.05954431826024},{"timestamp":1382138484,"value":50.10664600187493},{"timestamp":1382139500,"value":50.10331594410023},{"timestamp":1382140500,"value":50.0450940314545},{"timestamp":1382141500,"value":50.113955298362356},{"timestamp":1382142515,"value":50.08760336102611},{"timestamp":1382143515,"value":50.04651261202278},{"timestamp":1382144531,"value":50.02123062104736},{"timestamp":1382145531,"value":50.024536635083614},{"timestamp":1382146531,"value":50.012659028291544},{"timestamp":1382147531,"value":50.25957620025741},{"timestamp":1382148531,"value":50.52977973290773},{"timestamp":1382149546,"value":51.099760596496296},{"timestamp":1382150562,"value":51.233395694776284},{"timestamp":1382151562,"value":52.03920956690735},{"timestamp":1382152562,"value":52.37124157297022},{"timestamp":1382153562,"value":51.90628375891151},{"timestamp":1382154578,"value":51.93207066839429},{"timestamp":1382155593,"value":51.14861747301752},{"timestamp":1382156593,"value":50.91625878468039},{"timestamp":1382157609,"value":50.92489048678232},{"timestamp":1382158609,"value":50.918386655532814},{"timestamp":1382159625,"value":50.90904566314674},{"timestamp":1382160625,"value":49.49725745097422},{"timestamp":1382161640,"value":49.49230444085445},{"timestamp":1382162656,"value":49.47784213217949},{"timestamp":1382163656,"value":49.4610355590061},{"timestamp":1382164656,"value":49.42364754572338},{"timestamp":1382165671,"value":49.420509837856244},{"timestamp":1382166671,"value":49.46655359697934},{"timestamp":1382167687,"value":49.492328484592896},{"timestamp":1382168687,"value":49.50843778935136},{"timestamp":1382169703,"value":49.49615143900572},{"timestamp":1382170718,"value":49.39597320277263},{"timestamp":1382171718,"value":49.3769786494007},{"timestamp":1382172734,"value":49.35702234649095},{"timestamp":1382173734,"value":49.333796095152614},{"timestamp":1382174734,"value":49.374201597610245},{"timestamp":1382175734,"value":49.37679832136236},{"timestamp":1382176750,"value":49.617259749555615},{"timestamp":1382177750,"value":49.61775264619374},{"timestamp":1382178750,"value":49.6095897969915},{"timestamp":1382179781,"value":49.68949916171506},{"timestamp":1382180781,"value":49.51842796267546},{"timestamp":1382181781,"value":49.46065085919097},{"timestamp":1382182796,"value":49.48124432116953},{"timestamp":1382183796,"value":49.467226821655814},{"timestamp":1382184796,"value":49.44414483274815},{"timestamp":1382185812,"value":49.53073835675955},{"timestamp":1382186812,"value":49.55688592231901},{"timestamp":1382187828,"value":49.46353610780443},{"timestamp":1382188828,"value":49.416013658766936},{"timestamp":1382189843,"value":49.408331684333604},{"timestamp":1382190843,"value":49.41526830287513},{"timestamp":1382191843,"value":49.282606976002185},{"timestamp":1382192843,"value":49.27350642100057},{"timestamp":1382193859,"value":49.18656426278171},{"timestamp":1382194875,"value":49.11832813307343},{"timestamp":1382195890,"value":49.12334125253932},{"timestamp":1382196890,"value":49.12191065010181},{"timestamp":1382197906,"value":49.109996977702075},{"timestamp":1382198921,"value":49.08173356315942},{"timestamp":1382199937,"value":49.02424498453627},{"timestamp":1382200953,"value":49.03210728700794},{"timestamp":1382201953,"value":49.002281029466324},{"timestamp":1382202968,"value":48.996510532239405},{"timestamp":1382203984,"value":48.96896842985011},{"timestamp":1382204984,"value":48.93474216817297},{"timestamp":1382205984,"value":48.91040990486614},{"timestamp":1382206984,"value":48.86988418371628},{"timestamp":1382208000,"value":48.86459456125828},{"timestamp":1382209000,"value":48.8772415676806},{"timestamp":1382210000,"value":48.85182733614373},{"timestamp":1382211000,"value":48.85018034006021},{"timestamp":1382212000,"value":48.955972789220326},{"timestamp":1382213015,"value":48.93533123976488},{"timestamp":1382214015,"value":48.94403507308215},{"timestamp":1382215031,"value":48.95509519276707},{"timestamp":1382216031,"value":48.90249951491758},{"timestamp":1382217031,"value":48.88755633147371},{"timestamp":1382218031,"value":48.907212087652894},{"timestamp":1382219046,"value":48.926879865701295},{"timestamp":1382220062,"value":48.85940111375405},{"timestamp":1382221078,"value":48.83897595794462},{"timestamp":1382222078,"value":48.85793444570888},{"timestamp":1382223093,"value":48.80254969419971},{"timestamp":1382224093,"value":48.71185671278336},{"timestamp":1382225125,"value":48.69466543979484},{"timestamp":1382226125,"value":48.75961959920531},{"timestamp":1382227125,"value":48.74730920512122},{"timestamp":1382228140,"value":48.69804358504643},{"timestamp":1382229140,"value":48.737282966189454},{"timestamp":1382230140,"value":48.759222877520955},{"timestamp":1382231156,"value":48.74692450530609},{"timestamp":1382232171,"value":48.72485235341314},{"timestamp":1382233171,"value":48.77476715442596},{"timestamp":1382234171,"value":48.74134635798674},{"timestamp":1382235171,"value":48.765606490078234},{"timestamp":1382236203,"value":48.751144181403276},{"timestamp":1382237218,"value":48.74088952695627},{"timestamp":1382238234,"value":48.72362612275242},{"timestamp":1382239234,"value":48.738509196850174},{"timestamp":1382240250,"value":48.860110404038195},{"timestamp":1382241250,"value":48.90337711137084},{"timestamp":1382242250,"value":48.90298038968648},{"timestamp":1382243265,"value":48.91388422507151},{"timestamp":1382244265,"value":48.84079126019725},{"timestamp":1382245265,"value":48.849723509029744},{"timestamp":1382246281,"value":48.83262841099501},{"timestamp":1382247296,"value":48.8420896220733},{"timestamp":1382248296,"value":48.82525900516147},{"timestamp":1382249296,"value":48.84644153873194},{"timestamp":1382250312,"value":48.847343178923644},{"timestamp":1382251312,"value":48.85269291072776},{"timestamp":1382252312,"value":48.79473547920493},{"timestamp":1382253328,"value":48.77790486229309},{"timestamp":1382254328,"value":48.8120589927549},{"timestamp":1382255328,"value":48.798173733802635},{"timestamp":1382256328,"value":48.881268893870214},{"timestamp":1382257328,"value":48.86390931471258},{"timestamp":1382258343,"value":48.873214241490984},{"timestamp":1382259343,"value":48.85632351523303},{"timestamp":1382260343,"value":48.8325683016489},{"timestamp":1382261343,"value":48.78585131784933},{"timestamp":1382262343,"value":48.82368414029329},{"timestamp":1382263359,"value":48.79400214518235},{"timestamp":1382264375,"value":48.872276535691604},{"timestamp":1382265390,"value":48.61978121640639},{"timestamp":1382266406,"value":48.6342194813429},{"timestamp":1382267406,"value":48.80214095064614},{"timestamp":1382268406,"value":48.66443043869965},{"timestamp":1382269406,"value":48.634471940596576},{"timestamp":1382270406,"value":48.6019167187414},{"timestamp":1382271421,"value":48.628977946361786},{"timestamp":1382272437,"value":48.594703597207754},{"timestamp":1382273437,"value":48.420122012355115},{"timestamp":1382274437,"value":48.50545324009813},{"timestamp":1382275453,"value":48.7352993577677},{"timestamp":1382276453,"value":48.75267095879456},{"timestamp":1382277453,"value":48.764536543717405},{"timestamp":1382278468,"value":48.69407636820293},{"timestamp":1382279484,"value":48.687067618446065},{"timestamp":1382280484,"value":48.698812984676685},{"timestamp":1382281484,"value":48.68436269787095},{"timestamp":1382282500,"value":48.62724679719371},{"timestamp":1382283515,"value":48.63787412958661},{"timestamp":1382284515,"value":48.599812891627415},{"timestamp":1382285531,"value":48.55494727568815},{"timestamp":1382286546,"value":48.55326421399697},{"timestamp":1382287546,"value":48.65412769677576},{"timestamp":1382288546,"value":48.557339627663474},{"timestamp":1382289562,"value":48.49864886211805},{"timestamp":1382290562,"value":48.45704117273815},{"timestamp":1382291578,"value":48.48269584165948},{"timestamp":1382292578,"value":48.474352664418895},{"timestamp":1382293578,"value":48.473992008342215},{"timestamp":1382294578,"value":48.40787172761714},{"timestamp":1382295593,"value":48.466225880824325},{"timestamp":1382296593,"value":48.44741165549074},{"timestamp":1382297593,"value":48.3501186678711},{"timestamp":1382298609,"value":48.39433510287234},{"timestamp":1382299609,"value":48.39118537313598},{"timestamp":1382300609,"value":48.448373405028555},{"timestamp":1382301625,"value":48.474629167411024},{"timestamp":1382302625,"value":48.46764446139261},{"timestamp":1382303640,"value":48.46853407971509},{"timestamp":1382304640,"value":48.4847155156889},{"timestamp":1382305640,"value":48.47375157095776},{"timestamp":1382306656,"value":48.479425893230896},{"timestamp":1382307656,"value":48.55253087997438},{"timestamp":1382308671,"value":48.497158150334435},{"timestamp":1382309687,"value":48.492950496106474},{"timestamp":1382310687,"value":48.492938474237256},{"timestamp":1382311687,"value":48.50407072513751},{"timestamp":1382312703,"value":48.50682373318952},{"timestamp":1382313718,"value":48.50954067563386},{"timestamp":1382314718,"value":48.515431391553},{"timestamp":1382315734,"value":48.5159603537988},{"timestamp":1382316734,"value":48.4630400854803},{"timestamp":1382317734,"value":48.80299450336096},{"timestamp":1382318734,"value":48.71649715430333},{"timestamp":1382319734,"value":48.80270597849961},{"timestamp":1382320750,"value":48.925473307002235},{"timestamp":1382321750,"value":48.94768972132586},{"timestamp":1382322765,"value":48.80050597643184},{"timestamp":1382323765,"value":48.81427101669188},{"timestamp":1382324781,"value":48.83207540501076},{"timestamp":1382325781,"value":48.83856721439104},{"timestamp":1382326781,"value":48.78640432383357},{"timestamp":1382327781,"value":48.770511412721106},{"timestamp":1382328796,"value":48.7089714641699},{"timestamp":1382329796,"value":48.777207593878174},{"timestamp":1382330812,"value":48.71115944436844},{"timestamp":1382331828,"value":48.81550926922183},{"timestamp":1382332828,"value":48.78218464773639}]
```


## 14. 获取注册表快照
/api/registry/snapshot
```json
{"autoStart":[{"autoStartType":"BrowserHelpers","category":"Unknown","path":"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects","subkeys":["{004B0726-A010-4ABF-8556-FCDB7F1FCA1E}","{1FD49718-1D00-4B19-AF5F-070AF6D5D54C}"],"values":[]}],"network":[{"path":"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters","subkeys":["Adapters","DNSRegisteredAdapters","Interfaces","NsiObjectSecurity","PersistentRoutes","Winsock"],"values":[]},{"path":"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces","subkeys":["{01f344c7-716e-4838-be55-89d74914703b}","{0ebef201-45ad-4c0e-9602-433f91506ec9}","{0ee4f85a-dbf5-11ec-9eed-806e6f6e6963}","{1725e487-4ef6-4265-973f-fbf76e5438cb}","{3b82438f-5bd4-4543-aae0-3a22d5b8cd9d}","{3fdccc23-ec86-47bc-a310-cde1b3720f34}","{68ec1766-d242-4163-b63e-b4a39fa7c872}","{6be80337-ec55-4000-acfd-8ab9ce1943b8}","{7f23f83d-3a6e-4fa7-ae52-f6a03c088b95}","{cf133a80-3c41-457a-b21b-b235ecb24d35}"],"values":[]}],"software":[{"path":"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall","subkeys":["AddressBook","BiliBili","Connection Manager","DirectDrawEx","DXM_Runtime","Everything","Fontcore","Git_is1","HomeStudent2021Retail - zh-cn","IE40","IE4Data","IE5BAKEX","IEData","MiService","MobileOptionPack","MPlayer2","mstsc-4b0a31aa-df6a-4307-9b47-d5cc50009643","OneDriveSetup.exe","OneNoteFreeRetail - zh-cn","PotPlayer64","QEMU","SchedulingAgent","WeMeet","WIC","{075AD9D5-C301-4D8B-9549-5371F020B11F}","{0E992720-1330-4AB3-8155-255F79785535}","{1D8E6291-B0D5-35EC-8441-6616F567A0F7}","{2290438F-6CEF-4BAF-9618-FCE36F3434C0}","{235C9435-3313-4658-881D-5A7E559A5A41}","{27673300-A49E-0F4E-483E-8F5BA1BA1773}","{3873679C-FA03-4101-97E9-107D67C568B8}","{3E5195F5-ED93-4406-B149-F9F66F35E851}","{43B0D101-A022-48F4-9D04-BA404CEB1D53}","{51FD170D-69D2-4D5E-A9D1-53FD1F394FD7}","{5B7C4B06-7F22-461E-AABE-29E7719CB225}","{5C6E65EA-152C-472E-8538-D4D04F64B2B1}","{642E582D-9345-499A-BFCF-B966521A71AF}","{66B288E6-3354-AB0F-920D-909DDAA653FF}","{6BD2B618-9A2B-47D9-B24B-2F05BD2768E4}","{6C1EDC0A-E71D-44D2-821C-213A5CD15498}","{6DE9FE72-8A86-45DE-A689-049D43CBBF0B}","{6F320B93-EE3C-4826-85E0-ADF79F8D4C61}","{850cdc16-85df-4052-b06e-4e3e9e83c5c6}","{86AB2CC9-08BD-4643-B0F9-F82D006D72FF}","{8705254B-3AE0-4CFA-93D5-F71DCDE9ED2B}","{88D41995-0077-47CC-A2C0-149AD515C76A}","{8FFFFDF8-01C2-489B-A7E7-B9ABAF394C70}","{90160000-007E-0000-1000-0000000FF1CE}","{90160000-008C-0000-1000-0000000FF1CE}","{90160000-008C-0804-1000-0000000FF1CE}","{96A23710-E014-4A5B-96A1-DE64AC37251B}","{A139F43E-8105-465D-AC80-28F349CBE08D}","{A65B1339-6492-4CA4-AEB5-2B25A83A20B9}","{A891E3DB-0F4D-B1C1-EC88-CF1C8ABAF8CB}","{ACB8119F-54A6-C93C-CA2F-3A1476EE8869}","{AF2B4F8F-4F55-42E5-8208-9623B1AFA833}","{B4D3359E-1191-4BBD-ABDB-0E4A39534981}","{B4FD4106-3286-44D7-9B29-91A8B1D03328}","{B797608C-8536-46EF-95D8-8474139EA43A}","{BA39E81D-C8AE-41C0-91DC-81AF2BC439F4}","{BE75E968-78F4-411D-92E4-73EC3043F7E4}","{C53ECC06-72B1-FEBF-D241-038A4BC83E57}","{C6FD611E-7EFE-488C-A0E0-974C09EF6473}","{C762588C-06DE-496A-8ADF-EBDBD49E37B5}","{CD31E178-F872-466B-A231-9C8AA53A89FD}","{CFAAA24B-16EF-4D9D-80A5-F67798771571}","{D112F2CE-F362-616D-E13D-EA6A44AEDE6F}","{D5419286-34A7-E062-1C25-013A7FA94E9C}","{D9009790-F40A-C423-CCB8-65ECEC9A9FA9}","{DE8607D5-EA36-4398-AF76-25DD98D7DF8E}","{DF895F3B-AD64-DF8C-E7A3-8E079F4F9AB1}","{E4047598-558F-4468-8B53-9FCEF7F86E0D}","{E4AD9691-F2C7-5AC4-8795-3330B2AB4CC3}","{F563DC73-9550-F772-B4BF-2F72C83F9F30}","{F5796163-6EC6-488A-B2DE-E1E94477F6AD}","{FE85AA49-3522-4663-9F52-9CD9E9837189}"],"values":[]},{"path":"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall","subkeys":["fbd30ee5-8150-549e-9aed-fd9d444364fb","Feishu","JetBrains Toolbox (CLion) aedde446-5d32-4df0-9015-7af7517c35ce","JetBrains Toolbox (Goland) 9df1cec7-b9b9-41db-b117-3122761de0bc","Toolbox","{10b4f19a-24cb-442f-b93f-3e529a7007aa}","{771FD6B0-FA20-440A-A002-3B3BAC16DC50}_is1","{7dcc18b5-aff8-4c00-a38c-11f6344279b3}","{9a46f6d0-8f11-4f8f-a23d-d617db01bbb6}","{C3B34D50-AF22-4686-8CFE-4E88F2A73B39}","{c8d99ced-492c-458e-a343-d9316d918d5f}"],"values":[]}],"systemInfo":[{"path":"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion","subkeys":["Accessibility","AdaptiveDisplayBrightness","AeDebug","AppCompatFlags","ASR","BackgroundModel","ClipSVC","Compatibility32","Console","Containers","CorruptedFileRecovery","DefaultProductKey","DefaultProductKey2","DeviceDisplayObjects","DiskDiagnostics","drivers.desc","Drivers32","EFS","EMDMgmt","Event Viewer","Font Drivers","Font Management","FontDPI","FontIntensityCorrection","FontLink","FontMapper","FontMapperFamilyFallback","Fonts","FontSubstitutes","GRE_Initialize","HostComputeService","HumanPresence","ICM","Image File Execution Options","IniFileMapping","KnownFunctionTableDlls","KnownManagedDebuggingDlls","LanguagePack","LicensingDiag","MCI Extensions","MCI32","MiniDumpAuxiliaryDlls","MsiCorruptedFileRecovery","Multimedia","NaAuth","NetworkCards","NetworkList","NoImeModeImes","Notifications","NowPlayingSessionManager","NtVdm64","OEM","OpenGLDrivers","PasswordLess","Perflib","PerHwIdStorage","Ports","Prefetcher","Print","ProfileList","ProfileNotification","RemoteRegistry","ResourceManager","SBI","Schedule","SecEdit","Sensor","SoftwareProtectionPlatform","SPP","SRUM","Subscriptions","Superfetch","Svchost","SystemRestore","TileDataModel","Time Zones","TokenBroker","Tracing","UAC","UnattendSettings","Update","VersionsList","Virtualization","WbemPerf","WiFiDirectAPI","Windows","Winlogon","WinSAT","WinSATAPI","WirelessDocking","WOF","WorkloadManager","WUDF","ProfileService","VolatileNotifications"],"values":[]},{"path":"HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System","subkeys":["CentralProcessor","FloatingPointProcessor","MultifunctionAdapter","BIOS","VideoAdapterBusses"],"values":[]}],"timestamp":1382535703}
```

## 15 保存注册表
/api/registry/snapshot/save
POST方法
请求参数
{
  "snapshotName": "before_install" // snapshot名称
}

响应
```json
{
    "success": true
}
```

## 16 获取已保存的快照列表
接口说明: 获取所有已保存的注册表快照列表
请求URL: /api/registry/snapshots
请求方法: GET
响应
```json
[{"name":"before_install","networkCount":0,"softwareCount":0,"systemInfoCount":0,"timestamp":1383563468},{"name":"before_install_1","networkCount":0,"softwareCount":0,"systemInfoCount":0,"timestamp":1384427281},{"name":"before_install_2","networkCount":0,"softwareCount":0,"systemInfoCount":0,"timestamp":1384430718},{"name":"before_install_3","networkCount":0,"softwareCount":0,"systemInfoCount":0,"timestamp":1384433953}]

```



## 17 比较注册表快照
接口说明: 比较两个注册表快照的差异
请求URL: /api/registry/snapshots/compare
请求方法: POST
认证要求: 否
CORS: 支持
请求体: JSON
请求示例:
```json
{
  "snapshot1": "before_install",
  "snapshot2": "after_install"
}
```

响应示例:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "snapshot1": "before_install",
    "snapshot2": "after_install",
    "timestamp1": 1635427800000,
    "timestamp2": 1635431400000,
    "systemInfoChanges": {
      "added": ["HKEY_LOCAL_MACHINE\\SYSTEM\\NewKey"],
      "removed": ["HKEY_LOCAL_MACHINE\\SYSTEM\\OldKey"],
      "modified": ["HKEY_LOCAL_MACHINE\\SYSTEM\\ModifiedKey"]
    },
    "softwareChanges": {...},
    "networkChanges": {...}
  }
}
```



## 18 获取驱动快照
接口说明: 获取系统驱动程序快照
请求URL: /api/drivers/snapshot
请求方法: GET
认证要求: 否
CORS: 支持
响应示例:

```json
    {
        "statistics": {
            "autoStartCount": 106,
            "fileSystemCount": 37,
            "kernelCount": 408,
            "runningCount": 494,
            "stoppedCount": 228,
            "thirdPartyCount": 427,
            "totalDrivers": 999
        },
        "autoStartDrivers": [
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\ACPI.sys",
                "displayName": "Microsoft ACPI Driver",
                "name": "ACPI",
                "startType": "Boot",
                "state": "Running"
            },
            {
                "binaryPath": "\\SystemRoot\\System32\\Drivers\\acpiex.sys",
                "displayName": "Microsoft ACPIEx Driver",
                "name": "acpiex",
                "startType": "Boot",
                "state": "Running"
            }
        ],
        "fileSystemDrivers": [
            {
                "account": "Unknown",
                "binaryPath": "\\SystemRoot\\system32\\drivers\\bfs.sys",
                "description": "Unknown",
                "displayName": "中转文件系统",
                "driverType": "FileSystem",
                "errorControl": "Normal",
                "name": "bfs",
                "pid": 0,
                "serviceType": "File System Driver",
                "startType": "Auto",
                "state": "Running"
            },
            {
                "account": "Unknown",
                "binaryPath": "\\SystemRoot\\system32\\drivers\\bindflt.sys",
                "description": "Unknown",
                "displayName": "Windows Bind Filter Driver",
                "driverType": "FileSystem",
                "errorControl": "Normal",
                "name": "bindflt",
                "pid": 0,
                "serviceType": "File System Driver",
                "startType": "Auto",
                "state": "Running"
            }
        ],
        "kernelDrivers": [
            {
                "account": "Unknown",
                "binaryPath": "\\SystemRoot\\System32\\drivers\\1394ohci.sys",
                "description": "Unknown",
                "displayName": "1394 OHCI Compliant Host Controller",
                "driverType": "Kernel",
                "errorControl": "Normal",
                "name": "1394ohci",
                "pid": 0,
                "serviceType": "Kernel Driver",
                "startType": "Manual",
                "state": "Stopped"
            },
            {
                "account": "Unknown",
                "binaryPath": "\\SystemRoot\\System32\\drivers\\3ware.sys",
                "description": "Unknown",
                "displayName": "3ware",
                "driverType": "Kernel",
                "errorControl": "Normal",
                "name": "3ware",
                "pid": 0,
                "serviceType": "Kernel Driver",
                "startType": "Manual",
                "state": "Stopped"
            }
        ],
        "runningDrivers": [
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\ACPI.sys",
                "displayName": "Microsoft ACPI Driver",
                "name": "ACPI",
                "pid": 0,
                "startType": "Boot",
                "state": "Running"
            },
            {
                "binaryPath": "\\SystemRoot\\System32\\Drivers\\acpiex.sys",
                "displayName": "Microsoft ACPIEx Driver",
                "name": "acpiex",
                "pid": 0,
                "startType": "Boot",
                "state": "Running"
            },
            {
                "binaryPath": "\\SystemRoot\\System32\\DriverStore\\FileRepository\\acpipagr.inf_amd64_d1093347a27ff89c\\acpipagr.sys",
                "displayName": "ACPI 处理器聚合器驱动程序",
                "name": "acpipagr",
                "pid": 0,
                "startType": "Manual",
                "state": "Running"
            }
        ],
        "stoppedDrivers": [
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\1394ohci.sys",
                "displayName": "1394 OHCI Compliant Host Controller",
                "name": "1394ohci",
                "startType": "Manual",
                "state": "Stopped"
            },
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\3ware.sys",
                "displayName": "3ware",
                "name": "3ware",
                "startType": "Manual",
                "state": "Stopped"
            },
            {
                "binaryPath": "\\??\\C:\\Windows\\system32\\drivers\\ACE-BASE.sys",
                "displayName": "ACE-BASE",
                "name": "ACE-BASE",
                "startType": "Manual",
                "state": "Stopped"
            }
        ],
        "thirdPartyDrivers": [
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\1394ohci.sys",
                "displayName": "1394 OHCI Compliant Host Controller",
                "name": "1394ohci",
                "startType": "Manual",
                "state": "Stopped"
            },
            {
                "binaryPath": "\\SystemRoot\\System32\\drivers\\3ware.sys",
                "displayName": "3ware",
                "name": "3ware",
                "startType": "Manual",
                "state": "Stopped"
            },
            {
                "binaryPath": "\\??\\C:\\Program Files\\AntiCheatExpert\\SGuard\\x64\\plugins\\ACE-SSC-DRV64.sys",
                "displayName": "ACE-SSC-DRV64",
                "name": "ACE-SSC-DRV64",
                "startType": "Manual",
                "state": "Stopped"
            }
        ],
        "timestamp": 1385015703
    }
```

## 19 获取驱动详情
接口说明: 获取特定驱动程序的详细信息
请求URL: /api/drivers/detail
请求方法: GET
查询参数:
name: 驱动名称 (必需)
请求示例：
http://localhost:8080/api/drivers/detail?name=3ware
响应示例
{
    "account": "Unknown",
    "binaryPath": "\\SystemRoot\\System32\\drivers\\3ware.sys",
    "description": "Unknown",
    "displayName": "3ware",
    "driverType": "Kernel",
    "errorControl": "Normal",
    "exitCode": "Unknown",
    "group": "Unknown",
    "name": "3ware",
    "pid": 0,
    "serviceSpecificExitCode": "0",
    "serviceType": "Kernel Driver",
    "startType": "Manual",
    "state": "Stopped",
    "tagId": "Unknown",
    "win32ExitCode": "1077"
}