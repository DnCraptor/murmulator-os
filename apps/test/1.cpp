#include "m-os-api.h"
//можно обойтись без
//template<typename T> class ReadProcFn

//вот простой скелет policy (стратегия) по А. Александреску,
//которую, я пытался тебе напомнить:

class invoker
{
public:
    virtual unsigned char invoke(unsigned short a)=0;
    virtual ~invoker()=default;
};

template <class T> class device: public invoker
{
    using method_t =  unsigned char (T::*)(unsigned short) ;
    T *h;
    method_t  fn;

     public:

     ~device()=default;

     device()
     :h(nullptr)
     ,fn(nullptr)
     {

     }

     template<class U>
     device(const U &h_, unsigned char(U::*_fn)(unsigned short))
     :h((T*)&h_)
     ,fn ((method_t)_fn)
     {

     }

     virtual unsigned char invoke(unsigned short a)
     {
        return (h->*fn)(a);
     }
};


class CPU {
    unsigned char reg;
    unsigned char ram;
public:
    CPU(): reg(1), ram(2) {}
    unsigned char readRegister(unsigned short a) {
        printf("CPU::readRegister[%d]->%d\n", a, reg);
        return reg;
    }
    unsigned char readRAM(unsigned short a) {
        printf("CPU::readRAM[%d]->%d\n", a, ram);
        return ram;
    }
};

class FDD {
    unsigned char reg;
    unsigned char io;
public:
    FDD(): reg(6), io(7) {}

    unsigned char readReg(unsigned short a) {
        printf("FDD::readReg[%d]->%d\n", a, reg);
        return reg;
    }
    unsigned char readIO(unsigned short a) {
        printf("FDD::readIO[%d]->%d\n", a, io);
        return io;
    }
};

int main(void) {
    // notmal flow, type-safe
   CPU cpu;
   device<CPU> dcpu(cpu, &CPU::readRegister);
   invoker *pdcpu=&dcpu;
   FDD fdd;
   device<FDD> dfdd(fdd, &FDD::readReg);
   invoker *pdfdd=&dfdd;

   invoker *inv_collection[]={pdcpu, pdfdd};
   for(int i=0; i<2; ++i)inv_collection[i]->invoke(i);
   printf("\n");
   //the following is equal to use operator= thrice + temporary obj created:
   std::swap(inv_collection[0], inv_collection[1]);

   for(int i=0; i<2; ++i)inv_collection[i]->invoke(i);
   printf("\n");

   //or just
   inv_collection[0]=inv_collection[1];
   for(int i=0; i<2; ++i)inv_collection[i]->invoke(i);

   return 0;
}
/** тут всё прозрачно и не требует приведений. Да, в случае дин. связывания в рантайме
будет доступ через указатель в vtable, но комфорт и безопасность не бывают бесплатными)
Главное, что ты присваиваешь не разные типы, а один тип интерфейсного уровня. Любой прочтёт и
сможет ковырять, если придётся. Да и ты через пару месяцев, вернёшся к этому коду без напряга.
Если будут девайсы с другой структурой, - легко специализировать device<uniq_case> для любого случая.
А вот если понадобятся другие методы - придётся добавить в invoke чисто виртуальные прототипы.
Реализовывать их надо будет во всех наследниках, но это не сложно)
*/