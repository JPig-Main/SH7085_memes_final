#include <stdlib.h>
#include "iodefine.h"
#include "typedefine.h"

#define printf  ((int (*)(const char *,...))0x00007c7c)

#define SW6     (PD.DR.BIT.B18)
#define SW5     (PD.DR.BIT.B17)
#define SW4     (PD.DR.BIT.B16)

#define LED6    (PE.DR.BIT.B11)
#define LED_ON  (0)
#define LED_OFF (1)

#define DIG1    (PE.DR.BIT.B3)
#define DIG2    (PE.DR.BIT.B2)
#define DIG3    (PE.DR.BIT.B1)

#define LCD_RS      (PA.DR.BIT.B22)
#define LCD_E       (PA.DR.BIT.B23)
#define LCD_RW      (PD.DR.BIT.B23)
#define LCD_DATA    (PD.DR.BYTE.HH)

// �Q�[����̊�̐�
#define NMROF_ROCKS 6

enum STS{STOP, RUN, PAUSE1, PAUSE2, PAUSE3};

struct position {
    int x;
    int y;
    int active;
};

// ���O�ɐ����������y���W
int before_y_position = -1;

void wait_us(_UINT);
void LCD_inst(_SBYTE);
void LCD_data(_SBYTE);
void LCD_cursor(_UINT, _UINT);
void LCD_putch(_SBYTE);
void LCD_putstr(_SBYTE *);
void LCD_cls(void);
void LCD_init(void);

// --------------------
// -- �g�p����֐��Q --
// --------------------
void wait_us(_UINT us) {
    _UINT val;

    val = us * 10 / 16;
    if (val >= 0xffff)
        val = 0xffff;

    CMT0.CMCOR = val;
    CMT0.CMCSR.BIT.CMF &= 0;
    CMT.CMSTR.BIT.STR0 = 1;
    while (!CMT0.CMCSR.BIT.CMF);
    CMT0.CMCSR.BIT.CMF = 0;
    CMT.CMSTR.BIT.STR0 = 0;
}

void LCD_inst(_SBYTE inst) {
    LCD_E = 0;
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_E = 1;
    LCD_DATA = inst;
    wait_us(1);
    LCD_E = 0;
    wait_us(40);
}

void LCD_data(_SBYTE data) {
    LCD_E = 0;
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_E = 1;
    LCD_DATA = data;
    wait_us(1);
    LCD_E = 0;
    wait_us(40);
}

void LCD_cursor(_UINT x, _UINT y) {
    if (x > 15)
        x = 15;
    if (y > 1)
        y = 1;
    LCD_inst(0x80 | x | y << 6);
}

void LCD_putch(_SBYTE ch) {
    LCD_data(ch);
}

void LCD_putstr(_SBYTE *str) {
    _SBYTE ch;

    while (ch = *str++)
        LCD_putch(ch);
}

void LCD_cls(void) {
    LCD_inst(0x01);
    wait_us(1640);
}

void LCD_init(void) {
    wait_us(45000);
    LCD_inst(0x30);
    wait_us(4100);
    LCD_inst(0x30);
    wait_us(100);
    LCD_inst(0x30);

    LCD_inst(0x38);
    LCD_inst(0x08);
    LCD_inst(0x01);
    wait_us(1640);
    LCD_inst(0x06);
    LCD_inst(0x0c);
}

// --------------------------------------------
// -- �Q�[���p�̊֐��Q --

/**
 * �������ړ�
 * @param struct position* me :�����̏��
 * @return void
 */
void move_me(struct position *me)
{
    struct position old_position;

    old_position.x = me->x;
    old_position.y = me->y;

    if (AD0.ADDR0 < 0x4000) {
        // -- �W���C�X�e�B�b�N�� --
        me->y = 0;
    } else if (AD0.ADDR0 > 0xc000) {
        // -- �W���C�X�e�B�b�N�� --
        me->y = 1;
    }

    if (AD0.ADDR1 < 0x4000) {
        // -- �W���C�X�e�B�b�N�E --
        if (me->x + 1 <= 15) {
            me->x++;
        }
    } else if (AD0.ADDR1 > 0xc000) {
        // -- �W���C�X�e�B�b�N�� --
        if (me->x - 1 >= 0) {
            me->x--;
        }
    }

    if (old_position.x != me->x || old_position.y != me->y) {
        // -- �ړ������Ƃ� .. �Â��\�������� --
        LCD_cursor(old_position.x, old_position.y);
        LCD_putch(' ');
    }
    LCD_cursor(me->x, me->y);
    LCD_putch('>');
}


/**
 * ����ړ�
 * @param struct position rock[] :��̏��
 * @return int end_rock_num :�[�ɓ��B������̐�
 */
int move_rock(struct position rock[])
{
    // ���id(�z��ԍ�)
    int rock_id;
    int end_rock_num = 0;

    for (rock_id = 0; rock_id < NMROF_ROCKS; rock_id++) {
        if (rock[rock_id].active) {
            // LCD��Ɋ₪���݂���ꍇ
            LCD_cursor(rock[rock_id].x, rock[rock_id].y);
            LCD_putch(' ');
            if (rock[rock_id].x == 0) {
                // �[�ɓ��B���������
                rock[rock_id].active = 0;
                end_rock_num++;
            } else {
                // �������Ɉړ�
                rock[rock_id].x--;
                LCD_cursor(rock[rock_id].x, rock[rock_id].y);
                LCD_putch('*');
            }
        }
    }

    return end_rock_num;
}

/**
 * �V������𐶐�
 * @param struct position rock[] :��̏��
 * @return void
 */
void new_rock(struct position rock[])
{
    int i, j;

    for (i = 0; i < NMROF_ROCKS; i++) {
        if (rock[i].active == 0) {
            // �V������̍��W��������
            rock[i].x = 15;
            rock[i].y = rand() % 2;
            // ���O�̊��y���W�Əd�Ȃ�ɂ�������
            if (rock[i].y == before_y_position)
                rock[i].y = rand() % 2;
            // ���O�̊��y���W�Ƃ��Ċo���Ă���
            before_y_position = rock[i].y;

            // �������Ȃ�����Ȃ���(�΂ߓ��m�̊�͔������Ȃ�)
            for (j = 0; j < NMROF_ROCKS; j++) {
                if (j == i)
                    continue;
                if (rock[j].x == 14 && rock[j].y != rock[i].y)
                    return;
            }

            // �V��������쐬
            rock[i].active = 1;
            LCD_cursor(rock[i].x, rock[i].y);
            LCD_putch('*');
            return;
        }
    }
}

/**
 * �����Ɗ�Ƃ̏Փ˔���
 * @param struct position me :�����̏��
 * @param struct position rock[] :��̕��@
 * @return int rock_id|-1 :�Փ˂��Ă�������id(�z��ԍ�), �Փ˂��Ă��Ȃ�������-1
*/
int is_collision_rock (struct position me, struct position rock[]) {
    int rock_id;

    for (rock_id = 0; rock_id < NMROF_ROCKS; rock_id++) {
        if (!rock[rock_id].active)
            // LCD��Ɋ₪���݂��Ȃ��ꍇ
            continue;
        if (me.x == rock[rock_id].x && me.y == rock[rock_id].y)
            // ����������1�̊₪�Փ˂����ꍇ
            return rock_id;
    }

    return -1;
}

/**
 * ���_��\��
 * @param int score :���_
 * @return void
*/
void print_score (int score) {
    int score_workval;

    // LED6�Ő�����\��
    if (score >= 0) {
        score_workval = score;
        LED6 = LED_OFF;
    }
    else {
        score_workval = -score;
        LED6 = LED_ON;
    }

    // 1�̈ʂ�\��
    DIG1 = 1;
    DIG2 = 0;
    DIG3 = 0;
    PA.DR.BYTE.HL &= 0xF0;
    PA.DR.BYTE.HL |= score_workval % 10;
    wait_us(5000);

    // 10�̈ʂ�\��
    score_workval /= 10;
    if (score_workval == 0)
        // 1�̈ʂ����œ��_���\���������ꍇ
        return;
    DIG1 = 0;
    DIG2 = 1;
    PA.DR.BYTE.HL &= 0xF0;
    PA.DR.BYTE.HL |= score_workval % 10;
    wait_us(5000);

    // 100�̈ʂ�\��
    score_workval /= 10;
    if (score_workval == 0)
        // 100�̈ʂȂ��œ��_���\���������ꍇ
        return;
    DIG2 = 0;
    DIG3 = 1;
    PA.DR.BYTE.HL &= 0xF0;
    PA.DR.BYTE.HL |= score_workval;
    wait_us(5000);
}

// --------------------------------------------
// -- ���C���֐� --
void main() {
    // �����̎Ԃ̍��W
    struct position me;
    // ��̍��W
    struct position rock[NMROF_ROCKS];
    int move_timing, new_timing;
    int ad, i;
    int stop_sw;
    int run_sw;
    int pause_sw; // near ����
    int status;

    // ���id(�z��ԍ�)
    int rock_id;
    // �[�ɓ��B������̐�(1�񂲂Ƃ̃��[�v)
    int end_rock_num;

    // ���_
    int score;
    // ���x��(���x�����オ��Ƒ��x�������Ȃ�)
    int level = 1;
    // ���x�����Ƃ̑��x
    int speed[] = {62500, 46875, 31250, 23437.5, 15625, 11718.75, 7812.5};

    // LED6
    PFC.PEIORL.BIT.B11 = 1;

    // 7�Z�O�����gLED
    PFC.PEIORL.BIT.B3 = 1;
    PFC.PAIORH.BYTE.L |= 0x0F;

    STB.CR4.BIT._AD0 = 0;
    STB.CR4.BIT._CMT = 0;
    STB.CR4.BIT._MTU2 = 0;

    CMT0.CMCSR.BIT.CKS = 1;

    // MTU2 ch0
    MTU20.TCR.BIT.TPSC = 3;         // 1/64�I��
    MTU20.TCR.BIT.CCLR = 1;         // TGRA�̃R���y�A�}�b�`�ŃN���A
    MTU20.TGRA = 31250 - 1;         // 100ms
    MTU20.TIER.BIT.TTGE = 1;        // A/D�ϊ��J�n�v��������

    // AD0
    AD0.ADCSR.BIT.ADM = 3;          // 2�`���l���X�L�������[�h
    AD0.ADCSR.BIT.CH = 1;           // AN0, AN1
    AD0.ADCSR.BIT.TRGE = 1;         // MTU2����̃g���K�L��
    AD0.ADTSR.BIT.TRG0S = 1;        // TGRA�R���y�A�}�b�`�Ńg���K


    // MTU2 ch1
    MTU21.TCR.BIT.TPSC = 3;         // 1/64�I��
    MTU21.TCR.BIT.CCLR = 1;         // TGRA�̃R���y�A�}�b�`�ŃN���A
    MTU21.TGRA = speed[level - 1] - 1;         // 200ms

    LCD_init();

    while (1) {
        // MTU2 CH0�X�^�[�g
        MTU2.TSTR.BIT.CST0 = 1;
        // MTU2 CH1�X�^�[�g
        MTU2.TSTR.BIT.CST1 = 1;

        // �ϐ���������
        me.x = me.y = 0;
        for (rock_id = 0; rock_id < NMROF_ROCKS; rock_id++)
            rock[rock_id].active = 0;

        status = STOP;
        move_timing = new_timing = 0;
        score = 0;

        // �J�n�O�̕\��
        LCD_cursor(0, 0);
        LCD_putstr("Start the Game  ");
        LCD_cursor(0, 1);
        LCD_putstr("Push SW6        ");
        LED6 = LED_OFF;
        // �J�n�ҋ@
        while(!SW6)
            print_score(score);
        LCD_cls();

        while (1) {
            // �Q�[���p�̃^�C�}�̃t���O
            if (MTU21.TSR.BIT.TGFA) {
                MTU21.TSR.BIT.TGFA = 0;

                // ��莞�Ԃ��ƂɃX�C�b�`��ǂ�
                stop_sw = SW4;
                pause_sw = SW5;
                run_sw = SW6;

                // SW4: �Q�[���̏I��
                if (stop_sw) {
                    status = STOP;
                    break;
                }

                if (status == RUN) {
                    // �Q�[�����̏ꍇ
                    // �������ړ�
                    move_me(&me);
                    if (move_timing++ >= 2) {
                        move_timing = 0;
                        // ����ړ�
                        end_rock_num = move_rock(rock);
                        // �[�ɓ��B������ɉ����ē��_��ǉ�
                        score += end_rock_num;
                        if (new_timing-- <= 0) {
                            new_timing = rand() * 5 / (RAND_MAX + 1);
                            // �V�����₪�o��
                            new_rock(rock);
                        }
                    }
                    // SW5: �Q�[���̈ꎞ��~
                    if (pause_sw)
                        status = PAUSE1;
                } else if (status == PAUSE1)
                    // pause_sw �� OFF �Ȃ�status �� PAUSE2 ��
                    if (!pause_sw)
                        status = PAUSE2;
                else if (status == PAUSE2)
                    // pause_sw �� ON �Ȃ�status �� PAUSE3 ��
                    if (pause_sw)
                        status = PAUSE3;
                else if (status == PAUSE3)
                    // pause_sw �� OFF �Ȃ�status �� RUN ��
                    if (!pause_sw)
                        status = RUN;
                else
                    // ��~���̏ꍇ
                    // SW5: �Q�[���̍ĊJ
                    if (run_sw)
                        status= RUN;

            }

            if ((rock_id = is_collision_rock(me, rock)) != -1) {
                // �����Ɗ₪�Փ˂����ꍇ
                // �������
                rock[rock_id].active = 0;
                score -= 5;
            }
            // ���_�̕\��
            print_score(score);

            // 999�_�ŃN���A
            if (score >= 999) {
                LCD_cursor(0, 0);
                LCD_putstr("Clear the Game  ");
                LCD_cursor(0, 1);
                LCD_putstr("Push SW4        ");
                while(!SW4)
                    ;
                // �{�^���̉������Ԃ̔�����
                wait_us(500);
                break;
            }

            // -999�_�����ɂȂ�Ȃ��悤�ɂ���
            if (score < -999)
                score = -999;

            // ���_�ɉ����Ċ₪����鑬�x��ύX
            if (score < 10)
                level = 1;
            else if (score < 20)
                level = 2;
            else if (score < 40)
                level = 3;
            else if (score < 70)
                level = 4;
            else if (score < 110)
                level = 5;
            else if (score < 160)
                level = 6;
            else
                level = 7;
            MTU21.TGRA = speed[level - 1] - 1;
        }
    }
}
