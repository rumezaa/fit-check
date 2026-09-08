/* Pixel-art icons. Each is a bitmap on a square grid; runs of set pixels are
   merged per row into one path so edges stay on the grid at any size. */

type Bitmap = string[]

export const CAMERA: Bitmap = [
  '................',
  '................',
  '.....#####......',
  '.....#####......',
  '..############..',
  '.###########..#.',
  '.####.....#####.',
  '.###.......####.',
  '.###...##..####.',
  '.###...##..####.',
  '.###.......####.',
  '.####.....#####.',
  '.##############.',
  '..############..',
  '................',
  '................',
]

export const WAND: Bitmap = [
  '................',
  '............#...',
  '...........###..',
  '...#......#####.',
  '..###......###..',
  '...#......###...',
  '.........##.....',
  '........##....#.',
  '.......##....###',
  '......##......#.',
  '.....##.........',
  '....##..........',
  '...##...........',
  '..##............',
  '.##.............',
  '................',
]

export const BASKET: Bitmap = [
  '................',
  '................',
  '................',
  '......##........',
  '.....####..##...',
  '....##########..',
  '...###########..',
  '..############..',
  '.##############.',
  '.##############.',
  '..############..',
  '..##.##.##.###..',
  '...##########...',
  '...##.##.##.#...',
  '....########....',
  '................',
]

export const TRASH: Bitmap = [
  '................',
  '......####......',
  '..############..',
  '..############..',
  '...##########...',
  '...##########...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##.##.##.#...',
  '...##########...',
  '....########....',
  '................',
]

export const HEART: Bitmap = [
  '................',
  '................',
  '..###......###..',
  '.#####....#####.',
  '################',
  '################',
  '################',
  '.##############.',
  '..############..',
  '...##########...',
  '....########....',
  '.....######.....',
  '......####......',
  '.......##.......',
  '................',
  '................',
]

/** The two-arrow refresh ring, generated so the arcs and heads stay true. */
function refreshBitmap(N = 32): Bitmap {
  const C = N / 2, R_OUT = 11.6, R_IN = 7.6, R_MID = (R_OUT + R_IN) / 2
  const D2R = Math.PI / 180
  const cuts: [number, number][] = [[8, 62], [188, 242]]
  const heads = [62, 242]

  const tri = (deg: number) => {
    const rad = deg * D2R
    const E = [C + R_MID * Math.cos(rad), C - R_MID * Math.sin(rad)]
    const n = [Math.cos(rad), -Math.sin(rad)]        // radial
    const d = [Math.sin(rad), Math.cos(rad)]         // clockwise tangent
    const L = 6.4, W = 5.8
    return [
      [E[0] + d[0] * L, E[1] + d[1] * L],
      [E[0] + n[0] * W, E[1] + n[1] * W],
      [E[0] - n[0] * W, E[1] - n[1] * W],
    ] as number[][]
  }
  const inTri = (p: number[], t: number[][]) => {
    const s = (a: number[], b: number[], c: number[]) =>
      (a[0] - c[0]) * (b[1] - c[1]) - (b[0] - c[0]) * (a[1] - c[1])
    const d1 = s(p, t[0], t[1]), d2 = s(p, t[1], t[2]), d3 = s(p, t[2], t[0])
    return !(((d1 < 0) || (d2 < 0) || (d3 < 0)) && ((d1 > 0) || (d2 > 0) || (d3 > 0)))
  }
  const tris = heads.map(tri)

  const rows: Bitmap = []
  for (let y = 0; y < N; y++) {
    let row = ''
    for (let x = 0; x < N; x++) {
      const px = x + 0.5, py = y + 0.5
      const dx = px - C, dy = C - py
      const r = Math.hypot(dx, dy)
      let a = Math.atan2(dy, dx) / D2R; if (a < 0) a += 360
      let on = r >= R_IN && r <= R_OUT
      if (on) for (const [s0, e0] of cuts) if (a > s0 && a < e0) on = false
      if (!on) for (const t of tris) if (inTri([px, py], t)) { on = true; break }
      row += on ? '#' : '.'
    }
    rows.push(row)
  }
  return rows
}

export const REFRESH: Bitmap = refreshBitmap()

export const ICONS = {
  camera: CAMERA, wand: WAND, basket: BASKET,
  trash: TRASH, heart: HEART, refresh: REFRESH,
} as const

export type IconName = keyof typeof ICONS

/** Merge each row's runs into one SVG path — far fewer nodes than per-pixel. */
export function toPath(rows: Bitmap): string {
  const N = rows.length
  let d = ''
  for (let y = 0; y < N; y++) {
    const r = rows[y]
    let x = 0
    while (x < N) {
      if (r[x] !== '#') { x++; continue }
      let x2 = x
      while (x2 < N && r[x2] === '#') x2++
      d += `M${x} ${y}L${x2} ${y}L${x2} ${y + 1}L${x} ${y + 1}Z`
      x = x2
    }
  }
  return d
}

export function gridOf(name: IconName): number { return ICONS[name].length }
