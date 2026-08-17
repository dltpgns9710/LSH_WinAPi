# D01 구조 및 흐름 정리

`D01`은 `dungeon_01_01.json`(그리드/셀 배치)과 `sprite_texture_map.json`(오브젝트 모양)을
조합해 던전 한 층을 렌더링하는 `GameLevel`. 파츠 구성은 `ObjectEditor`, 타일 그리드/카메라
이동은 `Dungeon`을 참고해서 만들어졌다 (D01.h:18-20).

관련 파일:
- `Level/D01.h`, `Level/D01.cpp`
- `Data/DungeonData.h` — dungeon json 스키마
- `Data/SpriteTextureMapData.h` — sprite_texture_map json 스키마
- `D2DFramework/Camera/include/Camera.h`, `.../src/Camera.cpp`

---

## 1. 데이터 소스 두 개

D01은 서로 다른 두 JSON을 **이름(name/spriteSetIndex)으로 연결**해서 쓴다. 이 둘을 분리해서
이해하는 게 코드 흐름 파악의 핵심이다.

### 1-1. DungeonData (`dungeon_01_01.json`) — "어디에 뭘 놓을지"

```
DungeonData
├─ dungeonId, floorIndex, floorLabel
├─ startPosition (GridPosition)
├─ grid (GridInfo: width, height, cellSize)
├─ chipLegend            // chipNo -> 이름 문자열. 조회 없이 보관만 함
├─ spritePalette         // 파싱만 되고 코드 어디서도 안 읽는 미사용 필드
├─ spriteSets: map<int, vector<SpriteSetPart>>
│    SpriteSetPart = { name, blockRot }
│    → FindSpriteSet(index)로 조회
└─ cells: vector<Cell>
     Cell = { index, x, y, chipNo, chipName, cellRot, spriteSetIndex, stair?, treasure? }
```

- `Cell`은 그리드 한 칸. `chipName`("FLOOR"/"WALL"/"TREASURE"/"DOWN_STAIR" 등)과
  `spriteSetIndex`(이 칸에 놓을 스프라이트 조합), `cellRot`(칸 전체의 90도 단위 회전)을 가짐.
- `spriteSets[spriteSetIndex]`는 **스프라이트 이름 목록**일 뿐이다. 실제 텍스처 정보는 없음
  → 2단계로 `SpriteTextureMapData`에서 이름으로 재조회해야 함.

### 1-2. SpriteTextureMapData (`sprite_texture_map.json`) — "그 스프라이트가 실제로 어떤 텍스처인지"

```
SpriteTextureMapData
├─ texturelist: vector<TextureListEntry>      // { index, texture, texturePath }
└─ sprites: vector<Sprite>                    // { name, position, textures[] }
     TextureEntry = {
       textureIndex?,      // texturelist 인덱스. null이면 헬퍼(BoundingBox 등), 렌더 안 함
       textureWidth/Height,
       tiled, repeatX, repeatY,
       atlasRectPx?,       // 아틀라스 안에서 크롭할 픽셀 사각형
       position, size,     // 로컬 좌표/크기 (cm)
       rotationY, rotationX,  // 도(degree) 단위 추가 회전
       layer?              // 같은 셀 안 그리기 순서
     }
```

- `Sprite::name`이 `SpriteSetPart::name`과 매칭되는 키.
- `FindTexture(textureIndex)`로 `texturelist`에서 실제 파일 경로를 얻음.

---

## 2. Load() 흐름 (D01.cpp:65-105)

1. 배경(`Background.png`) 로드.
2. `DungeonData`, `SpriteTextureMapData` 두 JSON을 `DataManager`로 로드.
3. `dungeonData.cells`를 순회하며 `PlaceCell(cell)` 호출 → `placedParts`에 월드 배치 확정.
4. `RefreshDynamicWallFacing(startPosition)`으로 시작 위치 기준 얇은 벽 경사 방향 1차 결정.
5. 카메라 `nearZ/farZ`를 던전용 값으로 늘림(복도가 길어서 기본값으론 잘림).
6. 카메라 위치를 `startPosition`에 맞춤(`GridToWorld` + 눈높이 `kCameraHeight`).
7. DirectWrite 리소스 준비(던전 이름/디버그 텍스트 표시용).

---

## 3. 셀 하나를 "그림"으로 바꾸는 파이프라인

```
Cell (dungeon json)
  │  spriteSetIndex
  ▼
FindSpriteSet(spriteSetIndex)          [DungeonData.h:100]
  │  vector<SpriteSetPart{name, blockRot}>
  ▼
BuildCellParts(gridX, gridY, spriteSetIndex, effectiveCellRot)   [D01.cpp:209]
  │  각 SpriteSetPart마다:
  │    quarterTurns = (effectiveCellRot + blockRot) % 4
  │    GetOrBuildSpriteTemplate(name) 호출 →
  ▼
GetOrBuildSpriteTemplate(spriteName)   [D01.cpp:111, 캐시: spriteTemplateCache]
  │  spriteMapData.sprites 에서 name 매칭되는 Sprite 탐색
  │  Sprite.textures(TextureEntry) 각각을:
  │    - FindTexture(textureIndex)로 실제 파일 경로 획득
  │    - 크기 기반으로 isFloor / degenerate 판정
  │    - SpriteSheetManager로 텍스처 로드
  │    - CreateSubRegion / CreateTiledRegion으로 atlasRectPx만큼 잘라 SpriteSheet 생성
  │      (croppedSheetCache로 동일 크롭 재사용 캐시)
  │    - layer 결정(JSON 명시 or 크기 휴리스틱)
  ▼
PartTemplate 목록 (셀 회전과 무관한 "원본 모양")
  │  BuildCellParts에서 각 PartTemplate에 회전(quarterTurns * 90°) + 셀 월드좌표 적용
  ▼
PlacedPart 목록 (월드 좌표/회전이 확정된 최종 그리기 단위)
  │  PlaceCell()이 여기까지 실행하고 결과를 placedParts에 append
  ▼
D01::placedParts (Load() 시점에 던전 전체 1회 생성, 이후 Render()가 매 프레임 그대로 소비)
```

**캐시 2단계**:
- `spriteTemplateCache`: 스프라이트 이름 → `PartTemplate[]` (스프라이트 단위)
- `croppedSheetCache`: (파일명+atlasRect+tiled설정) → 크롭된 `SpriteSheet` (텍스처 조각 단위,
  서로 다른 스프라이트 이름이 같은 크롭을 공유하는 경우까지 커버)

---

## 4. 동적 벽(DynamicWallCell) — 경사 방향이 카메라 위치에 따라 바뀌는 얇은 벽

일부 얇은 벽은 경사 장식(rotationX≠0인 파츠)이 한쪽 방향에만 있는데, 그 벽이 N/S 또는 E/W
양쪽 모두에서 걸어 들어갈 수 있는 위치라면, 카메라가 서 있는 쪽으로 경사가 보이도록 `cellRot`을
런타임에 바꿔야 자연스럽다.

```
PlaceCell(cell)
  └─ chipName == "WALL" 이면 → TryRegisterDynamicWallCell(cell, ...)
       ├─ GetDistinctSlopeDirections(spriteSetIndex)  // 경사가 향하는 로컬 방향들(0=N,1=E,2=S,3=W)
       ├─ 인접 4방향 walkable 여부 체크 (IsWalkable)
       ├─ "경사 1개 + 정확히 N/S 또는 E/W 반대쌍만 걸을 수 있음" 케이스만 등록
       └─ dynamicWallCells 에 negative/positive side용 cellRot 두 값 저장

RefreshDynamicWallFacing(cameraGridX, cameraGridY)
  └─ dynamicWallCells 순회하며 카메라가 어느 쪽에 있는지로 desiredCellRot 결정
       → 바뀌었으면 BuildCellParts로 재계산해 placedParts[해당 구간]을 덮어씀
  └─ 호출 시점: Load() 끝(시작 위치 기준), Update()의 tryMove 성공 시(도착 칸 기준)
```

즉 `placedParts`는 대부분 Load()에서 고정되지만, 동적 벽에 해당하는 구간만 이동할 때마다
in-place로 교체된다.

---

## 5. 매 프레임 흐름

### Update(deltaTime) — 그리드 이동/회전 입력 처리 (D01.cpp:538)

```
super::Update / GetCamera()->Update(deltaTime)
if (!GetCamera()->IsIdle()) return;   // 이전 이동/회전 애니메이션 중이면 새 입력 무시

W/S/Q/E → tryMove(dir, delta)
   dest = 현재 카메라 위치 + delta(한 칸 분량)
   grid로 역변환 → IsWalkable 체크 → 통과 시:
     GetCamera()->moveRequest(dir)
     RefreshDynamicWallFacing(도착 칸)   // 도착 칸 기준으로 얇은 벽 갱신

A/D → GetCamera()->rotateRequest(dir); facingQuarter 갱신
```

- `Camera`는 `moveRequest`/`rotateRequest`가 idle이 아니면 요청을 조용히 무시한다
  (Camera.cpp:170, 198). D01은 `IsIdle()`을 프레임 시작에서 먼저 확인해서, 이 함수들이
  호출되는 시점엔 항상 idle이도록 보장한다 → 요청이 씹혀 `facingQuarter`(D01이 믿는 방향)와
  카메라의 실제 방향이 어긋나는 버그를 원천 차단.
- `facingQuarter`: 0=+Z 기준 90도 단위 방향. 이동 방향/디버그 표시 계산에 쓰임.

### Render() (D01.cpp:352)

1. 배경 전체 클리어 그리기(이전 프레임/레벨 잔상 제거).
2. 던전 이름 텍스트.
3. `placedParts` 순회하며:
   - `Camera::isRenderPosition`(값싼 사전 필터) → `computeCorners`로 4모서리 계산
     → `Camera::isRenderPoint`로 프러스텀 컬링, `viewProj`로 clip-space 깊이 계산.
4. 정렬(`std::sort`): 진짜 바닥 우선 → 셀 중심 깊이(cellDepth) 내림차순 → 같은 셀이면 layer
   오름차순 → 같은 layer면 코너 depth로 타이브레이크.
5. 정렬된 순서대로 `Matrix4x4` model 행렬 구성 후 `SpriteSheet::DrawSpriteWarped`.
6. 우측 상단에 카메라 위치/이동 방향 디버그 텍스트.

---

## 6. 좌표계 / 회전 규칙 요약

- 그리드 → 월드: `GridToWorld(x,y) = (x*cellSize, 0, -y*cellSize)` (D01.cpp:59).
  z가 grid y와 반대 부호인 이유는 카메라 기본 방향(+Z, facingQuarter=0)이 시작 위치에서
  계단을 등지는 방향과 맞아떨어지게 하기 위함.
- `facingQuarter`(0=+Z, A가 +1, D가 -1(=+3))와 `Camera`의 `theta`가 별도로 관리되지만,
  `QuarterToForward`가 `Camera::moveRequest`의 `forward=(sin(-theta),0,cos(-theta))` 규칙과
  동일하게 맞춰져 있어 서로 어긋나지 않음.
- 셀 회전 합성: `effectiveCellRot(=cell.cellRot 또는 동적 벽의 desiredCellRot) + SpriteSetPart.blockRot`
  → 90도 단위 총 회전. 파츠 로컬 좌표(`RotateY`)와 파츠 자체 Y회전(`worldYRotation`) 둘 다에 적용.
- `isFloor` 판정 기준(`GetOrBuildSpriteTemplate`): `size.y <= 0.8 * min(size.x, size.z)`이면 바닥류.
- `layer`: 같은 셀 안에서 그리는 순서(작을수록 먼저=뒤). JSON에 명시 없으면 셀보다 큰 파츠(오버사이즈
  배경/받침)는 0, 나머지는 1로 자동 분류.

---

## 7. 핵심 자료구조 요약

| 구조체 | 소속 | 의미 |
|---|---|---|
| `Cell` | DungeonData | 그리드 한 칸의 배치 정보 |
| `SpriteSetPart{name, blockRot}` | DungeonData | 칸에 놓을 스프라이트 이름 + 개별 회전 |
| `Sprite{name, textures[]}` | SpriteTextureMapData | 스프라이트 이름 → 텍스처 조각들 |
| `TextureEntry` | SpriteTextureMapData | 텍스처 조각 하나의 원본 스펙(atlasRect, 위치/크기/회전/layer) |
| `PartTemplate` | D01 (private) | `TextureEntry`를 로드/크롭까지 마친 재사용 가능한 형태 (셀 무관) |
| `PlacedPart` | D01 (private) | `PartTemplate`에 셀 월드좌표/회전을 적용해 확정한 최종 그리기 단위 |
| `DynamicWallCell` | D01 (private) | 카메라 위치에 따라 cellRot을 바꿔야 하는 얇은 벽 정보 |
