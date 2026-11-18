// These append to the original carbine, shotgun and coil rifle as slots 4–6.
export const EXTRA_WEAPONS = [
  {
    name: 'KITE-7 COMPACT', short: 'SMG', rarity: 'RARE · GREENLINE',
    color: 0x79e0cf, mag: 36, reserve: 216, damage: 15, rate: .085,
    reload: 1.5, spread: .026, pellets: 1, range: 58,
    description: 'A friendly-faction compact: fast bursts for close streets. Wide spread limits long shots.'
  },
  {
    name: 'LONGWATCH DMR', short: 'DMR', rarity: 'RARE · SURVEY ISSUE',
    color: 0xe4bd83, mag: 12, reserve: 84, damage: 60, rate: .48,
    reload: 1.9, spread: .0025, pellets: 1, range: 145,
    description: 'A reclaimed survey rifle. Slow, precise shots reward careful aim at machine optics.'
  },
  {
    name: 'BASTION-70', short: 'LMG', rarity: 'EPIC · DEFENSE RESERVE',
    color: 0xf1d276, mag: 70, reserve: 280, damage: 19, rate: .12,
    reload: 3.2, spread: .023, pellets: 1, range: 100,
    description: 'A belt-fed defensive weapon. Long sustained fire, a heavy box magazine, and a slow reload.'
  }
];

/** Add a distinct upper assembly to the common receiver and grips.
 * Coordinates use the existing gun-local convention: muzzle points toward -Z.
 * Call before the existing gun material/render-order traversal.
 */
export function decorateExtraWeapon({ kind, THREE, gun, box, cylinder, mat }) {
  if (kind < 3 || kind > 5) return null;
  const spec = EXTRA_WEAPONS[kind - 3];
  const graphite = mat(0x273c35, .65, .65);
  const wornSteel = mat(0x829085, .5, .8);
  const grip = mat(0x4c4d3d, .95, .1);
  const optic = new THREE.MeshStandardMaterial({
    color: spec.color, emissive: spec.color, emissiveIntensity: .6,
    roughness: .3, metalness: .25
  });
  const black = mat(0x11251f, .65, .55);
  const tube = (radius, length, x, y, z, material = wornSteel) => {
    const part = cylinder(radius, radius, length, x, y, z, material, gun, 10);
    part.rotation.x = Math.PI / 2;
    return part;
  };

  if (kind === 3) {
    // Compact ventilated handguard, wrapped suppressor and a narrow reflex sight.
    box(.205, .16, .32, 0, .035, -.34, graphite, gun);
    tube(.045, .23, 0, .05, -.56);
    tube(.066, .27, 0, .05, -.75, graphite);
    for (let i = 0; i < 4; i++) {
      box(.008, .065, .035, .105, .055, -.23 - i * .065, black, gun);
      box(.008, .065, .035, -.105, .055, -.23 - i * .065, black, gun);
    }
    box(.15, .045, .16, 0, .19, -.08, black, gun);
    for (const x of [-.059, .059]) box(.024, .1, .025, x, .25, -.12, graphite, gun);
    box(.14, .022, .025, 0, .3, -.12, graphite, gun);
    box(.016, .016, .007, 0, .248, -.104, optic, gun);
    const magazine = box(.11, .34, .13, 0, -.3, -.16, grip, gun);
    magazine.rotation.x = -.12;
    box(.012, .045, .22, .111, .02, .02, optic, gun);
    return { muzzleZ: -.92, adsFov: 57 };
  }

  if (kind === 4) {
    // Long free-floating barrel with a weathered wooden fore-end and scope.
    const timber = mat(0x776046, .9, .08);
    box(.2, .15, .48, 0, -.015, -.29, timber, gun);
    tube(.034, .59, 0, .05, -.72);
    tube(.053, .12, 0, .05, -1.045, graphite);
    box(.1, .05, .34, 0, .19, -.05, graphite, gun);
    for (const z of [-.15, .04]) box(.06, .085, .035, 0, .23, z, graphite, gun);
    tube(.064, .37, 0, .29, -.08, graphite);
    tube(.082, .09, 0, .29, -.28, graphite);
    tube(.074, .065, 0, .29, .12, graphite);
    tube(.052, .007, 0, .29, .156, optic);
    const dial = cylinder(.033, .033, .045, .073, .29, -.04, wornSteel, gun, 8);
    dial.rotation.z = Math.PI / 2;
    box(.045, .035, .11, .13, .07, .08, wornSteel, gun);
    box(.018, .12, .015, .15, .015, .13, black, gun);
    for (const x of [-.08, .08]) {
      const leg = box(.025, .025, .36, x, -.12, -.5, wornSteel, gun);
      leg.rotation.y = x < 0 ? -.08 : .08;
    }
    return { muzzleZ: -1.16, adsFov: 39 };
  }

  // A wide stamped cover, perforated cooling jacket and visible linked rounds.
  const olive = mat(0x626748, .9, .3);
  const brass = mat(0xc49b54, .45, .75);
  box(.28, .17, .51, 0, .065, -.15, olive, gun);
  tube(.072, .45, 0, .045, -.58, graphite);
  tube(.037, .18, 0, .045, -.86);
  tube(.053, .08, 0, .045, -.96, graphite);
  for (let i = 0; i < 6; i++) {
    box(.014, .028, .025, .068, .065, -.4 - i * .06, black, gun);
    box(.014, .028, .025, -.068, .065, -.4 - i * .06, black, gun);
  }
  box(.29, .32, .32, -.115, -.21, -.08, olive, gun);
  box(.31, .04, .34, -.115, -.07, -.08, wornSteel, gun);
  for (let i = 0; i < 5; i++) {
    tube(.016, .115, -.14 - i * .035, -.015 - i * .006, -.07, brass);
    box(.015, .015, .08, -.14 - i * .035, -.015 - i * .006, -.07, graphite, gun);
  }
  // Carry handle and high front post make the heavy weapon recognizable.
  for (const z of [-.2, -.02]) box(.025, .1, .025, .045, .21, z, wornSteel, gun);
  box(.045, .035, .22, .045, .26, -.11, grip, gun);
  box(.04, .15, .045, 0, .16, -.65, graphite, gun);
  box(.012, .022, .014, 0, .248, -.65, optic, gun);
  box(.045, .06, .07, 0, .23, .12, graphite, gun);
  return { muzzleZ: -1.08, adsFov: 56 };
}
